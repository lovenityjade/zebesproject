#pragma once
#include <stdint.h>
#include <time.h>
extern uint64_t sm_profile_ns[5];
#ifdef _WIN32
uint64_t sm_windows_clock_ns(void);
static inline uint64_t sm_clock_ns(void) { return sm_windows_clock_ns(); }
#else
static inline uint64_t sm_clock_ns(void) {
  struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);
  return (uint64_t)t.tv_sec*1000000000ull+t.tv_nsec;
}
#endif
