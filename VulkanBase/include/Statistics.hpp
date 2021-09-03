#pragma once
#include "common.h"

namespace stats {

    template<typename T>
    struct Statistics {
        T minValue{};
        T maxValue{};
        T medianValue{};
        float meanValue{};
        float variance{};
        float standardDeviation{};
    };

    template<typename T, typename DataSet>
    inline Statistics<T> summarize(DataSet& dataSet){
        auto N = dataSet.size();
        assert(N > 1);

        std::sort(begin(dataSet), end(dataSet));

        T minValue{dataSet.front()};
        T maxValue{dataSet.back()};
        int middle =(N/2);
        T medianValue{dataSet[middle]};

        if(N%2 == 0){
            medianValue = medianValue + dataSet[middle - 1];
            medianValue *= 0.5;
        }

        float meanValue = std::accumulate(begin(dataSet), end(dataSet), T{})/N;
        float variance = std::accumulate(begin(dataSet), end(dataSet), 0.0f, [avg=meanValue](auto acc, auto val){
            return acc + (val - avg) * (val - avg);
        });
        variance /= static_cast<float>(N);

        float standardDeviation{std::sqrt(variance)};

        return { minValue, maxValue, medianValue, meanValue, variance, standardDeviation};
    }

}