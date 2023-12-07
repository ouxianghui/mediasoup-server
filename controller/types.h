/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#pragma once

#include <memory>
#include <string>
#include <atomic>
#include <chrono>
#include "nlohmann/json.hpp"
#include "asio.hpp"
#include "srv_logger.h"
#include "FBS/request.h"
#include "FBS/response.h"

namespace srv {

    using namespace std::chrono_literals;

    class IError 
    {
    public:
        virtual ~IError() = default;

        virtual const std::string& name() const = 0;
        virtual const std::string& message() const = 0;
        virtual const std::string& stack() const = 0;
    };

    class Error : public IError
    {
    public:
        Error(const std::string& name, const std::string& message, const std::string& stack = "")
        : _name(name)
        , _message(message)
        , _stack(stack) {}

        const std::string& name() const override { return _name; }
        const std::string& message() const override { return _message; };
        const std::string& stack() const override { return _stack; }
        
    private:
        std::string _name;
        std::string _message;
        std::string _stack;
    };

    class ChannelError : public std::runtime_error
    {
    public:
        explicit ChannelError(const char* description) : std::runtime_error(description) {}
    };

    class Callback : public std::enable_shared_from_this<Callback>
    {
    public:
        using ResolveFn = std::function<void(const std::vector<uint8_t>&)>;
        using RejectFn = std::function<void(const IError&)>;
        using TimeoutFn = std::function<void()>;
        using CloseFn = std::function<void()>;
        
    public:
        //Callback(uint32_t id_, FBS::Request::Method method, const ResolveFn& resolve, const RejectFn& reject, const CloseFn& close, const TimeoutFn& timeout)
        Callback(uint32_t id_, const ResolveFn& resolve, const RejectFn& reject, const CloseFn& close, const TimeoutFn& timeout)
        : _id(id_)
        //, _method(method)
        , _resolve(resolve)
        , _reject(reject)
        , _close(close)
        , _timeout(timeout) {}
        
        ~Callback() {
            if (_timer) {
                _timer->cancel();
            }
        }
        
        uint32_t id() { return _id; }
        
        FBS::Request::Method method() { return _method; }
        
        void resolve(const std::vector<uint8_t>& data) {
            if (_timer) {
                _timer->cancel();
            }
            if (!_closed) {
                _resolve ? _resolve(data) : void();
            }
        }
        
        void reject(const IError& error) {
            if (_timer) {
                _timer->cancel();
            }
            if (!_closed) {
                _reject ? _reject(error) : void();
            }
        }
        
        void close() {
            if (_timer) {
                _timer->cancel();
            }
            _closed = true;
            _close ? _close() : void();
        }
        
        void setTimeout(asio::static_thread_pool& context, uint32_t duration) {
            _timer = std::make_shared<asio::steady_timer>(context, std::chrono::milliseconds(duration));
            _timer->async_wait([wself = std::weak_ptr<Callback>(shared_from_this())](const asio::system_error& error){
                if (error.code().value() != 0) {
                    return;
                }
                auto self = wself.lock();
                if (!self) {
                    return;
                }
                if (self->_timer) {
                    self->_timer->cancel();
                }
                self->_timeout ? self->_timeout() : void();
            });
        }
        
    private:
        uint32_t _id;
        FBS::Request::Method _method;
        ResolveFn _resolve;
        RejectFn _reject;
        CloseFn _close;
        TimeoutFn _timeout;
        std::atomic_bool _closed { false };
        std::shared_ptr<asio::steady_timer> _timer;
    };

    class MediaSoupError : public std::runtime_error
    {
    public:
        explicit MediaSoupError(const char* description) : std::runtime_error(description) {}

    public:
        static const size_t bufferSize { 2000 };
        thread_local static char buffer[];
    };

    class MediaSoupTypeError : public MediaSoupError
    {
    public:
        explicit MediaSoupTypeError(const char* description) : MediaSoupError(description) {}
    };

    class WebsocketRequest : public std::enable_shared_from_this<WebsocketRequest>
    {
    public:
        using ResolveFn = std::function<void(const nlohmann::json&)>;
        using RejectFn = std::function<void(const IError&)>;
        using TimeoutFn = std::function<void()>;
        using CloseFn = std::function<void()>;
    
    public:
        WebsocketRequest(int64_t id) : _id(id) {}

        ~WebsocketRequest() {
            if (_timer) {
                _timer->cancel();
            }
        }

        int64_t id() {
            return _id;
        }

        void setData(const nlohmann::json& data) {
            _data = data;
        }

        const nlohmann::json& data() {
            return _data;
        }

        const std::shared_ptr<asio::steady_timer>& timer() {
            return _timer;
        }

        void setTimeout(asio::static_thread_pool& context, uint32_t duration, const TimeoutFn& timeout) {
            _timeout = timeout;
            _timer = std::make_shared<asio::steady_timer>(context);
            _timer->expires_after(std::chrono::milliseconds(duration));
            _timer->async_wait([wself = std::weak_ptr<WebsocketRequest>(shared_from_this())](const asio::system_error& error){
                if (error.code().value() != 0) {
                    return;
                }
                auto self = wself.lock();
                if (!self) {
                    return;
                }
                if (self->_timer) {
                    self->_timer->cancel();
                }
                self->_timeout ? self->_timeout() : void();
            });
        }

        void setResolveFn(const ResolveFn& resolve) {
            _resolve = resolve;
        }

        void setRejectFn(const RejectFn& reject) {
            _reject = reject;
        }

        void resolve(const nlohmann::json& data) {
            if (_timer) {
                _timer->cancel();
            }
            if (!_closed && _resolve) {
                _resolve(data);
            }
        }

        void reject(const IError& error) {
            if (_timer) {
                _timer->cancel();
            }
            if (!_closed && _reject) {
                _reject(error);
            }
        }

        void close() {
            _closed = true;
            if (_timer) {
                _timer->cancel();
            }
            if (_close) {
                _close();
            }
        }

    private:
        int64_t _id = -1;
        nlohmann::json _data;
        ResolveFn _resolve;
        RejectFn _reject;
        TimeoutFn _timeout;
        CloseFn _close;
        std::atomic_bool _closed { false };
        std::shared_ptr<asio::steady_timer> _timer;
    };
}

#define _SRV_LOG_SEPARATOR_CHAR_STD "\n"

#ifdef SRV_LOG_FILE_LINE
    #define _SRV_LOG_STR " %s:%d | %s::%s()"
    #define _SRV_LOG_STR_DESC _SRV_LOG_STR " | "
    #define _SRV_FILE (std::strchr(__FILE__, '/') ? std::strchr(__FILE__, '/') + 1 : __FILE__)
    #define _SRV_LOG_ARG _SRV_FILE, __LINE__, SRV_CLASS, __FUNCTION__
#else
    #define _SRV_LOG_STR " %s::%s()"
    #define _SRV_LOG_STR_DESC _SRV_LOG_STR " | "
    #define _SRV_LOG_ARG SRV_CLASS, __FUNCTION__
#endif

#define SRV_ERROR(desc, ...) \
    do { \
        printf("[ERROR]" _SRV_LOG_STR_DESC desc, _SRV_LOG_ARG, ##__VA_ARGS__); \
    } while (false)

#define SRV_ERROR_STD(desc, ...) \
    do { \
            std::fprintf(stderr, _SRV_LOG_STR_DESC desc _SRV_LOG_SEPARATOR_CHAR_STD, _SRV_LOG_ARG, ##__VA_ARGS__); \
            std::fflush(stderr); \
    } while (false)

#define SRV_THROW_ERROR(desc, ...) \
    do \
    { \
        SRV_ERROR("throwing MediaSoupError: " desc, ##__VA_ARGS__); \
        std::snprintf(srv::MediaSoupError::buffer, srv::MediaSoupError::bufferSize, desc, ##__VA_ARGS__); \
        throw MediaSoupError(srv::MediaSoupError::buffer); \
    } while (false)

#define SRV_THROW_ERROR_STD(desc, ...) \
    do \
    { \
        SRV_ERROR_STD("throwing MediaSoupError: " desc, ##__VA_ARGS__); \
        std::snprintf(srv::MediaSoupError::buffer, srv::MediaSoupError::bufferSize, desc, ##__VA_ARGS__); \
        throw MediaSoupError(srv::MediaSoupError::buffer); \
    } while (false)

#define SRV_THROW_TYPE_ERROR(desc, ...) \
    do \
    { \
        SRV_ERROR("throwing MediaSoupTypeError: " desc, ##__VA_ARGS__); \
        std::snprintf(srv::MediaSoupError::buffer, srv::MediaSoupError::bufferSize, desc, ##__VA_ARGS__); \
        throw MediaSoupTypeError(srv::MediaSoupError::buffer); \
    } while (false)

#define SRV_THROW_TYPE_ERROR_STD(desc, ...) \
    do \
    { \
        SRV_ERROR_STD("throwing MediaSoupTypeError: " desc, ##__VA_ARGS__); \
        std::snprintf(srv::MediaSoupError::buffer, srv::MediaSoupError::bufferSize, desc, ##__VA_ARGS__); \
        throw MediaSoupTypeError(srv::MediaSoupError::buffer); \
    } while (false)


#define SRV_ABORT(desc, ...) \
    do \
    { \
        std::fprintf(stderr, "(ABORT) " _SRV_LOG_STR_DESC desc _SRV_LOG_SEPARATOR_CHAR_STD, _SRV_LOG_ARG, ##__VA_ARGS__); \
        std::fflush(stderr); \
        std::abort(); \
    } \
    while (false)

#define SRV_ASSERT(condition, desc, ...) \
    if (!(condition)) \
    { \
        SRV_ABORT("failed assertion `%s': " desc, #condition, ##__VA_ARGS__); \
    }
