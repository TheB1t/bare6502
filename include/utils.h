#pragma once

#include <time.h>

static inline double get_time_in_nanoseconds() {
    struct timespec tv;
    clock_gettime(CLOCK_MONOTONIC, &tv);
    return (double)((tv.tv_sec * 1e9) + tv.tv_nsec);
}
