#pragma once

#include "common.h"

struct Particle{
    glm::vec4 position{0};
    glm::vec4 color{0};
    glm::vec3 velocity{0};
    float invMass{0};
};

constexpr int ITEMS_PER_WORKGROUP = 8 << 10;