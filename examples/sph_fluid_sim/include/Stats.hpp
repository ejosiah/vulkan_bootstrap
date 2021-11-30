#pragma once

#include "common.h"

struct TimeStamps{
    uint64_t buildHashGridStartTime;
    uint64_t buildHashGridEndTime;
    uint64_t buildNeighbourListStartTime;
    uint64_t buildNeighbourListEndTime;
};

struct Stat{
    float value{0};
    int n{0};

    void update(uint64_t startTime, uint64_t endTime){
        n++;
        auto duration = float(endTime - startTime);
        value = mix(value, duration, 1.0f/float(n));
    }
};

struct Stats{
    Stat buildHashGridStat;
    Stat buildNeighbourListStat;
};