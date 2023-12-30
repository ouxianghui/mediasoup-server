/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#include "utils.h"
#include <assert.h>
#include "srv_logger.h"

namespace
{
    inline static void onCloseLoop(uv_handle_t* handle)
    {
        delete reinterpret_cast<uv_loop_t*>(handle);
    }

    inline static void onWalk(uv_handle_t* handle, void* /*arg*/)
    {
        // Must use MS_ERROR_STD since at this point the Channel is already closed.
        SRV_LOGD("alive UV handle found (this shouldn't happen) [type:%s, active:%d, closing:%d, has_ref:%d]", 
                 uv_handle_type_name(handle->type),
                 uv_is_active(handle),
                 uv_is_closing(handle),
                 uv_has_ref(handle));
        
        if (!uv_is_closing(handle)) {
            uv_close(handle, onCloseLoop);
        }
    }
}

namespace srv {

    Loop::Loop()
    {
        _loop = new uv_loop_t();
        assert(_loop);
        
        uv_loop_init(_loop);
    }

    Loop::~Loop()
    {
        if (_loop) {
            int err;
            uv_stop(_loop);
            uv_walk(_loop, onWalk, nullptr);
            
            while (true) {
                err = uv_loop_close(_loop);
                if (err != UV_EBUSY) {
                    break;
                }
                uv_run(_loop, UV_RUN_NOWAIT);
            }
            
            if (err != 0) {
                SRV_LOGE("failed to close libuv loop: %s", uv_err_name(err));
            }
            
            delete _loop;
            _loop = nullptr;
            
            if (_thread.joinable()) {
                _thread.join();
            }
        }
    }

    void Loop::asyncRun()
    {
        _thread = std::thread([this](){
            uv_run(this->_loop, uv_run_mode::UV_RUN_DEFAULT);
        });
    }

    void Loop::run()
    {
        uv_run(this->_loop, uv_run_mode::UV_RUN_DEFAULT);
    }

}
