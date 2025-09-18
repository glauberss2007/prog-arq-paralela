#include "CycleTimer.h"
#include <sys/time.h>
#include <x86intrin.h>

double CycleTimer::currentSeconds() {
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return ((double)tp.tv_sec + (double)tp.tv_usec * 1e-6);
}

uint64_t CycleTimer::currentCycles() {
    return __rdtsc();
}