/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#pragma once

#include <string>
#include <atomic>
#include <chrono>
#include "nlohmann/json.hpp"
#include "asio.hpp"

namespace srv {

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

    class Callback
    {
    public:
        using Resolve = std::function<void(const nlohmann::json&)>;
        using Reject = std::function<void(const IError&)>;
        using Timeout = std::function<void()>;
        using Close = std::function<void()>;
        
    public:
        Callback(uint32_t id_, const std::string& method, const Resolve& resolve, const Reject& reject, const Close& close, const Timeout& timeout)
        : _id(id_)
        , _method(method)
        , _resolve(resolve)
        , _reject(reject)
        , _close(close)
        , _timeout(timeout) {}
        
        uint32_t id() { return _id; }
        
        std::string method() { return _method; }
        
        void resolve(const nlohmann::json& data) {
            if (!_closed) {
                _resolve ? _resolve(data) : void();
            }
        }
        
        void reject(const IError& error) {
            if (!_closed) {
                _reject ? _reject(error) : void();
            }
        }
        
        void close() {
            _closed = true;
            _close ? _close() : void();
        }
        
        void setTimeout(asio::static_thread_pool& context, uint32_t duration) {
            _timer = std::make_shared<asio::steady_timer>(context, std::chrono::milliseconds(duration));
            _timer->async_wait(std::bind(&Callback::timeout, this));
        }
        
    private:
        void timeout() {
            //_timeout ? _timeout() : void();
        }
        
    private:
        uint32_t _id;
        std::string _method;
        Resolve _resolve;
        Reject _reject;
        Close _close;
        Timeout _timeout;
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

}
