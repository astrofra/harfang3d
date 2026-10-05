#pragma once
#include <time.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <malloc.h>
#define alloca _alloca
struct timeval { long long tv_sec; long tv_usec; };
static inline int gettimeofday(struct timeval *tv, void *tz) {
    FILETIME ft;
    ULARGE_INTEGER ticks;
    (void)tz;
    GetSystemTimeAsFileTime(&ft);
    ticks.LowPart = ft.dwLowDateTime;
    ticks.HighPart = ft.dwHighDateTime;
    ticks.QuadPart -= 116444736000000000ULL;
    tv->tv_sec = ticks.QuadPart / 10000000ULL;
    tv->tv_usec = (long)((ticks.QuadPart % 10000000ULL) / 10);
    return 0;
}
