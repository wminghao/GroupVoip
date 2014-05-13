#ifndef __UNITS_H__
#define __UNITS_H__

//assume it's linux system
#include <stdint.h>
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

inline u32 MAX(u32 a, u32 b) {
    return a > b? a : b;
}
inline u32 MIN(u32 a, u32 b) {
    return a < b? a : b;
}

#define MAX_U32 0xffffffff

#ifdef DEBUG
#include <assert.h>
#ifdef ANDROID
#include "androidlog.h"
#define OUTPUT(...) android_syslog(ANDROID_LOG_DEBUG, __VA_ARGS__)
#define ASSERT(x) assert(x)
#else //for iPhone
#define OUTPUT(...) printf(__VA_ARGS__)
#define ASSERT(x) assert(x)
#endif //ANDROID or iphone
#else //DEBUG
#define OUTPUT(...) 
#define ASSERT(x) 
#endif //DEBUG

#endif
