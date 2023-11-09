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
        using ResolveFn = std::function<void(const nlohmann::json&)>;
        using RejectFn = std::function<void(const IError&)>;
        using TimeoutFn = std::function<void()>;
        using CloseFn = std::function<void()>;
        
    public:
        Callback(uint32_t id_, const std::string& method, const ResolveFn& resolve, const RejectFn& reject, const CloseFn& close, const TimeoutFn& timeout)
        : _id(id_)
        , _method(method)
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
        
        std::string method() { return _method; }
        
        void resolve(const nlohmann::json& data) {
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
        std::string _method;
        ResolveFn _resolve;
        RejectFn _reject;
        CloseFn _close;
        TimeoutFn _timeout;
        std::atomic_bool _closed { false };
        std::shared_ptr<asio::steady_timer> _timer;
    };
    
    struct StatBase
    {
        // Common to all RtpStreams.
        std::string type;
        int32_t timestamp;
        uint32_t ssrc;
        uint32_t rtxSsrc;
        std::string kind;
        std::string mimeType;
        int32_t packetsLost;
        int32_t fractionLost;
        int32_t packetsDiscarded;
        int32_t packetsRetransmitted;
        int32_t packetsRepaired;
        int32_t nackCount;
        int32_t nackPacketCount;
        int32_t pliCount;
        int32_t firCount;
        int32_t score;
        int32_t packetCount;
        int32_t byteCount;
        int32_t bitrate;
        int32_t roundTripTime;
    };

    class MediaSoupError : public std::runtime_error
    {
    public:
        explicit MediaSoupError(const char* description);
    };

    inline MediaSoupError::MediaSoupError(const char* description) : std::runtime_error(description) {}

    class MediaSoupTypeError : public MediaSoupError
    {
    public:
        explicit MediaSoupTypeError(const char* description);
    };

    inline MediaSoupTypeError::MediaSoupTypeError(const char* description) : MediaSoupError(description) {}

    #ifdef MSC_LOG_FILE_LINE
        #define _MSC_LOG_STR " %s:%d | %s::%s()"
        #define _MSC_LOG_STR_DESC _MSC_LOG_STR " | "
        #define _MSC_FILE (std::strchr(__FILE__, '/') ? std::strchr(__FILE__, '/') + 1 : __FILE__)
        #define _MSC_LOG_ARG _MSC_FILE, __LINE__, MSC_CLASS, __FUNCTION__
    #else
        #define _MSC_LOG_STR " %s::%s()"
        #define _MSC_LOG_STR_DESC _MSC_LOG_STR " | "
        #define _MSC_LOG_ARG MSC_CLASS, __FUNCTION__
    #endif

    #define MSC_ERROR(desc, ...) \
    do { \
        printf("[ERROR]" _MSC_LOG_STR_DESC desc, _MSC_LOG_ARG, ##__VA_ARGS__); \
    } \
    while (false)

    #define SRV_THROW_TYPE_ERROR(desc, ...) \
    do { \
        MSC_ERROR("throwing MediaSoupTypeError: " desc, ##__VA_ARGS__); \
        \
        static char buffer[2000]; \
        \
        std::snprintf(buffer, 2000, desc, ##__VA_ARGS__); \
        throw srv::MediaSoupTypeError(buffer); \
    } while (false)

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
