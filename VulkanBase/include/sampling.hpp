#pragma once
#include "common.h"

namespace sampling {

    inline glm::vec2 uniformSampleDisk(const glm::vec2& u){
        float r = std::sqrt(u[0]);
        float theta = glm::two_pi<float>() * u[1];
        return {r * std::cos(theta), r * std::sin(theta)};
    }

}