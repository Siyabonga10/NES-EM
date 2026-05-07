#ifndef DEBUG_LOG_H
#define DEBUG_LOG_H

#ifdef ANDROID_DEBUG_LOG
#include <android/log.h>
#define printf(fmt, ...) __android_log_print(ANDROID_LOG_INFO, "NesCore", fmt, ##__VA_ARGS__)
#elif defined(NES_CORE_SILENT)
#define printf(fmt, ...) ((void)0)
#endif

#endif
