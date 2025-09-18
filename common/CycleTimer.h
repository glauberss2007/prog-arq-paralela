#ifndef __CYCLETIMER_H__
#define __CYCLETIMER_H__

#include <stdint.h>

class CycleTimer {
public:
    static double currentSeconds();
    static uint64_t currentCycles();
};

#endif