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
#include <mutex>

namespace srv {

template<typename T>
class Singleton {
public:
    virtual ~Singleton() {}
    static std::shared_ptr<T>& sharedInstance()
    {
        static std::shared_ptr<T> _instance;
        static std::once_flag ocf;
        std::call_once(ocf, [](){
            _instance.reset(new T);
        });
        return _instance;
    }
};

}