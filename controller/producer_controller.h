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
#include <vector>
#include <atomic>
#include <string>
#include <unordered_map>
#include "nlohmann/json.hpp"
#include "sigslot/signal.hpp"
#include "types.h"
#include "rtp_parameters.h"

namespace srv {

    struct ProducerOptions
    {
        /**
         * Producer id (just for Router.pipeToRouter() method).
         */
        std::string id;

        /**
         * Media kind ('audio' or 'video').
         */
        std::string kind;

        /**
         * RTP parameters defining what the endpoint is sending.
         */
        RtpParameters rtpParameters;

        /**
         * Whether the producer must start in paused mode. Default false.
         */
        bool paused = false;

        /**
         * Just for video. Time (in ms) before asking the sender for a new key frame
         * after having asked a previous one. Default 0.
         */
       int32_t  keyFrameRequestDelay;

        /**
         * Custom application data.
         */
        nlohmann::json appData;
    };

    /**
     * Valid types for 'trace' event.
     */
    // export type ProducerTraceEventType = 'rtp' | 'keyframe' | 'nack' | 'pli' | 'fir';

    /**
     * 'trace' event data.
     */
    struct ProducerTraceEventData
    {
        /**
         * Trace type.
         * Options: 'rtp' | 'keyframe' | 'nack' | 'pli' | 'fir'
         */
        std::string type;

        /**
         * Event timestamp.
         */
        int64_t timestamp;

        /**
         * Event direction.
         * Options: 'in' | 'out'
         */
        std::string direction;

        /**
         * Per type information.
         */
        nlohmann::json info;
    };

    void to_json(nlohmann::json& j, const ProducerTraceEventData& st);
    void from_json(const nlohmann::json& j, ProducerTraceEventData& st);

    struct ProducerScore
    {
        /**
         * SSRC of the RTP stream.
         */
        uint32_t ssrc;

        /**
         * RID of the RTP stream.
         */
        std::string rid;

        /**
         * The score of the RTP stream.
         */
        int32_t score;
    };

    void to_json(nlohmann::json& j, const ProducerScore& st);
    void from_json(const nlohmann::json& j, ProducerScore& st);

    struct ProducerVideoOrientation
    {
        /**
         * Whether the source is a video camera.
         */
        bool camera;

        /**
         * Whether the video source is flipped.
         */
        bool flip;

        /**
         * Rotation degrees (0, 90, 180 or 270).
         */
        int32_t rotation;
    };

    void to_json(nlohmann::json& j, const ProducerVideoOrientation& st);
    void from_json(const nlohmann::json& j, ProducerVideoOrientation& st);


    struct ProducerStat : StatBase {};

    void to_json(nlohmann::json& j, const ProducerStat& st);
    void from_json(const nlohmann::json& j, ProducerStat& st);

    /**
     * Producer type.
     */
    // export type ProducerType = 'simple' | 'simulcast' | 'svc';

    struct ProducerInternal
    {
        std::string transportId;
        std::string producerId;
    };

    struct ProducerData
    {
        std::string kind;
        RtpParameters rtpParameters;
        std::string type;
        RtpParameters consumableRtpParameters;
    };

    class Channel;
    class PayloadChannel;

    class ProducerController : public std::enable_shared_from_this<ProducerController>
    {
    public:
        ProducerController(const ProducerInternal& internal,
                           const ProducerData& data,
                           const std::shared_ptr<Channel>& channel,
                           const std::shared_ptr<PayloadChannel>& payloadChannel,
                           const nlohmann::json& appData,
                           bool paused);
        
        virtual ~ProducerController();
        
        void init();
        
        void destroy();
        
        const std::string& id() { return _internal.producerId; }

        const std::string& kind() { return _data.kind; }

        const RtpParameters& rtpParameters() { return _data.rtpParameters; }

        const std::string& type() { return _data.type; }

        const RtpParameters& consumableRtpParameters() { return _data.consumableRtpParameters; }
        
        bool paused() { return _paused; }

        const std::vector<ProducerScore>& score() {
            std::lock_guard<std::mutex> lock(_scoreMutex);
            return _score;
        }

        bool closed() { return _closed; }
        
        void setAppData(const nlohmann::json& data) { _appData = data; }
        
        const nlohmann::json& appData() { return _appData; }
        
        void close();
        
        void onTransportClosed();
        
        nlohmann::json dump();
        
        nlohmann::json getStats();
        
        void pause();
        
        void resume();

        // types = 'rtp' | 'keyframe' | 'nack' | 'pli' | 'fir';
        void enableTraceEvent(const std::vector<std::string>& types);
        
        void send(const uint8_t* payload, size_t payloadLen);
        
    private:
        void handleWorkerNotifications();
        
        void onChannel(const std::string& targetId, const std::string& event, const std::string& data);
        
    public:
        sigslot::signal<> transportCloseSignal;
        
        sigslot::signal<const std::vector<ProducerScore>&> scoreSignal;
        
        sigslot::signal<const ProducerVideoOrientation&> videoOrientationChangeSignal;
        
        sigslot::signal<const ProducerTraceEventData&> traceSignal;
        
        sigslot::signal<> closeSignal;
        
        sigslot::signal<> pauseSignal;
        
        sigslot::signal<> resumeSignal;
        
    private:
        // Internal data.
        ProducerInternal _internal;

        // Consumer data.
        ProducerData _data;

        // Channel instance.
        std::weak_ptr<Channel> _channel;

        // PayloadChannel instance.
        std::weak_ptr<PayloadChannel> _payloadChannel;

        // Closed flag.
        std::atomic_bool _closed { false };

        // Custom app data.
        nlohmann::json _appData;

        // Paused flag.
        std::atomic_bool _paused { false };
        
        std::mutex _scoreMutex;
        std::vector<ProducerScore> _score;
        
    };

}
