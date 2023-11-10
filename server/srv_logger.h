/************************************************************************
* @Copyright: 2023-2024
* @FileName:
* @Description: Open source mediasoup C++ controller library
* @Version: 1.0.0
* @Author: Jackie Ou
* @CreateTime: 2023-10-30
*************************************************************************/

#pragma once

#include <stdio.h>

#if 1
#define SRV_LOGD(...) { \
    fprintf(stderr, "%s: Line %d:\t", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__); \
    fprintf(stderr,"\n"); \
}

#define SRV_LOGI(...) { \
    fprintf(stderr, "%s: Line %d:\t", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__); \
    fprintf(stderr,"\n"); \
}

#define SRV_LOGW(...) { \
    fprintf(stderr, "%s: Line %d:\t", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__); \
    fprintf(stderr,"\n"); \
}

#define SRV_LOGE(...) { \
    fprintf(stderr, "%s: Line %d:\t", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__); \
    fprintf(stderr,"\n"); \
}
#else
#define SRV_LOGD(...) 

#define SRV_LOGI(...) 

#define SRV_LOGW(...) 

#define SRV_LOGE(...) 
#endif
