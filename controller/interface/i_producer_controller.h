/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#pragma once

#include <vector>
#include <atomic>
#include <string>
#include <unordered_map>
#include "threadsafe_vector.hpp"
#include "nlohmann/json.hpp"
#include "sigslot/signal.hpp"
#include "types.h"
#include "rtp_stream.h"
#include "rtp_parameters.h"
#include "ortc.h"

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
        int32_t keyFrameRequestDelay;

        /**
         * Custom application data.
         */
        nlohmann::json appData;
    };

    /**
     * Valid types for 'trace' event.
     */
    // export type ProducerTraceEventType = 'rtp' | 'keyframe' | 'nack' | 'pli' | 'fir' | 'sr';

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
        std::shared_ptr<TraceInfo> info;
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
        uint8_t score;
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
        uint16_t rotation;
    };

    void to_json(nlohmann::json& j, const ProducerVideoOrientation& st);
    void from_json(const nlohmann::json& j, ProducerVideoOrientation& st);

    struct ProducerStat : RtpStreamRecvStats {};

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

    struct ProducerDump
    {
        std::string id;
        std::string kind;
        std::string type;
        RtpParameters rtpParameters;
        RtpMappingFbs rtpMapping;
        std::vector<RtpStreamDump> rtpStreams;
        std::vector<std::string> traceEventTypes;
        bool paused;
    };

    class IProducerController
    {
    public:
        virtual ~IProducerController() = default;
        
        virtual void init() = 0;
        
        virtual void destroy() = 0;
        
        virtual const std::string& id() = 0;

        virtual const std::string& kind() = 0;

        virtual const RtpParameters& rtpParameters() = 0;

        virtual const std::string& type() = 0;

        virtual const RtpParameters& consumableRtpParameters() = 0;

        virtual const std::threadsafe_vector<ProducerScore>& score() = 0;
        
        virtual void setAppData(const nlohmann::json& data) = 0;
        
        virtual const nlohmann::json& appData() = 0;
        
        virtual void close() = 0;
        
        virtual bool closed() = 0;

        virtual std::shared_ptr<ProducerDump> dump() = 0;
        
        virtual std::vector<std::shared_ptr<ProducerStat>> getStats() = 0;
        
        virtual void pause() = 0;
        
        virtual void resume() = 0;

        virtual bool paused() = 0;

        // types = 'rtp' | 'keyframe' | 'nack' | 'pli' | 'fir';
        virtual void enableTraceEvent(const std::vector<std::string>& types) = 0;
        
        virtual void onTransportClosed() = 0;
        
        virtual void send(const std::vector<uint8_t>& data) = 0;
        
    public:
        sigslot::signal<> transportCloseSignal;
        
        sigslot::signal<const std::vector<ProducerScore>&> scoreSignal;
        
        sigslot::signal<const ProducerVideoOrientation&> videoOrientationChangeSignal;
        
        sigslot::signal<const ProducerTraceEventData&> traceSignal;
        
        sigslot::signal<> closeSignal;
        
        sigslot::signal<> pauseSignal;
        
        sigslot::signal<> resumeSignal;
    };
}
