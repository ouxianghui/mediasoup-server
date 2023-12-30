/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#pragma once

#include <cstdint>
#include <ctime>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <thread>
#include <uv.h>

namespace srv {

    template<typename T>
    inline T getRandomInteger(T min, T max)
    {
        // Seed with time.
        static unsigned int seed = time(nullptr);

        // Engine based on the Mersenne Twister 19937 (64 bits).
        static std::mt19937_64 rng(seed);

        // Uniform distribution for integers in the [min, max) range.
        std::uniform_int_distribution<T> dis(min, max);

        return dis(rng);
    }

    class Loop
    {
    public:
        Loop();
        
        ~Loop();
        
        uv_loop_t* get() { return _loop; }
        
        void run();
        
        void asyncRun();
        
    private:
        uv_loop_t* _loop = nullptr;
        
        std::thread _thread;
    };

}
