#pragma once

#include "common.h"

struct StdKernel{
    float h, h2, h3, h5;
    
    inline StdKernel(): h{0}, h2(0), h3(0), h5(0){}

    inline explicit StdKernel(float radius) : h(radius), h2(h * h), h3(h2 * h), h5(h3 * h2){}

    inline float operator()(float distance) const {
        if (distance * distance >= h2) {
            return 0.0;
        } else {
            float x = 1.0 - distance * distance / h2;
            return 315.0f / (64.0f * glm::pi<float>() * h3) * x * x * x;
        }
    }

    [[nodiscard]]
    inline float firstDerivative(float distance) const {
        if (distance >= h) {
            return 0.0;
        } else {
            float x = 1.0 - distance * distance / h2;
            return -945.0f / (32.0f * glm::pi<float>() * h5) * distance * x * x;
        }
    }

    [[nodiscard]]
    inline glm::vec3 gradient(const glm::vec3& point) const {
        float dist = length(point);
        if(dist > 0.0){
            return gradient(dist, point / dist);
        }
        return glm::vec3(0);
    }

    [[nodiscard]]
    inline glm::vec3 gradient(float distance, const glm::vec3 distanceToCenter) const {
        return -firstDerivative(distance) * distanceToCenter;
    }

    [[nodiscard]]
    inline float secondDerivative(float distance) const {
        if (distance * distance >= h2) {
            return 0.0f;
        } else {
            double x = distance * distance / h2;
            return 945.0f / (32.0f * glm::pi<float>() * h5) * (1 - x) * (5 * x - 1);
        }
    }
};