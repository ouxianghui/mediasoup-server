/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#pragma once

#include <tuple>
#include <atomic>
#include <unordered_set>
#include <mutex>
#include "nlohmann/json.hpp"
#include "sigslot/signal.hpp"
#include "types.h"
#include "rtp_parameters.h"

namespace srv {
 
    /**
    * An object with the fields of the uv_rusage_t struct.
    *
    * - http://docs.libuv.org/en/v1.x/misc.html#c.uv_rusage_t
    * - https://linux.die.net/man/2/getrusage
    */
    struct WorkerResourceUsage 
    {
        /* eslint-disable camelcase */
        
        /**
        * User CPU time used (in ms).
        */
        uint64_t ru_utime = 0;
        
        /**
        * System CPU time used (in ms).
        */
        uint64_t ru_stime = 0;
        
        /**
        * Maximum resident set size.
        */
        uint64_t ru_maxrss = 0;
        
        /**
        * Integral shared memory size.
        */
        uint64_t ru_ixrss = 0;
        
        /**
        * Integral unshared data size.
        */
        uint64_t ru_idrss = 0;
        
        /**
        * Integral unshared stack size.
        */
        uint64_t ru_isrss = 0;
        
        /**
        * Page reclaims (soft page faults).
        */
        uint64_t ru_minflt = 0;
        
        /**
        * Page faults (hard page faults).
        */
        uint64_t ru_majflt = 0;
        
        /**
        * Swaps.
        */
        uint64_t ru_nswap = 0;
        
        /**
        * Block input operations.
        */
        uint64_t ru_inblock = 0;
        
        /**
        * Block output operations.
        */
        uint64_t ru_oublock = 0;
        
        /**
        * IPC messages sent.
        */
        uint64_t ru_msgsnd = 0;
        
        /**
        * IPC messages received.
        */
        uint64_t ru_msgrcv = 0;
        
        /**
        * Signals received.
        */
        uint64_t ru_nsignals = 0;
        
        /**
        * Voluntary context switches.
        */
        uint64_t ru_nvcsw = 0;
        
        /**
        * Involuntary context switches.
        */
        uint64_t ru_nivcsw = 0;
        
        /* eslint-enable camelcase */
    };

    struct WorkerDump
    {
        std::vector<std::string> webRtcServerIds;
        std::vector<std::string> routerIds;
        
        struct ChannelMessageHandlers
        {
            std::vector<std::string> channelRequestHandlers;
            std::vector<std::string> channelNotificationHandlers;
        };
        ChannelMessageHandlers channelMessageHandlers;
        
        struct LibUring
        {
            uint64_t sqeProcessCount;
            uint64_t sqeMissCount;
            uint64_t userDataMissCount;
        };
        
        std::shared_ptr<LibUring> liburing;
    };

    class IWebRtcServerController;
    class IRouterController;
    struct WebRtcServerOptions;

    class IWorkerController
    {
    public:
        virtual ~IWorkerController() = default;
        
        virtual void init() = 0;
        
        virtual void destroy() = 0;
        
        virtual void runWorker() = 0;
        
        virtual int pid() = 0;
        
        virtual void close() = 0;

        virtual bool closed() = 0;
        
        virtual void setAppData(const nlohmann::json& data) = 0;
        
        virtual  std::shared_ptr<IWebRtcServerController> webRtcServerController() = 0;
        
        virtual const nlohmann::json& appData() = 0;
        
        virtual std::shared_ptr<WorkerDump> dump() = 0;
        
        virtual std::shared_ptr<WorkerResourceUsage> getResourceUsage() = 0;
        
        virtual void updateSettings(const std::string& logLevel, const std::vector<std::string>& logTags) = 0;
        
        virtual std::shared_ptr<IWebRtcServerController> createWebRtcServerController(const std::shared_ptr<WebRtcServerOptions>& options, const nlohmann::json& appData) = 0;
        
        virtual std::shared_ptr<IRouterController> createRouterController(const std::vector<RtpCodecCapability>& mediaCodecs, const nlohmann::json& appData) = 0;
        
    public:
        sigslot::signal<> startSignal;

        sigslot::signal<> closeSignal;
        
        sigslot::signal<std::shared_ptr<IWebRtcServerController>> newWebRtcServerSignal;
        
        sigslot::signal<std::shared_ptr<IRouterController>> newRouterSignal;
    };
}
