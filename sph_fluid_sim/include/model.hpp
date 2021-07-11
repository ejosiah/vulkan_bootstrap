#pragma once

#include "common.h"
#include "Texture.h"

constexpr int ITEMS_PER_WORKGROUP = 8 << 10;

struct Particle{
    glm::vec4 position{0};
    glm::vec4 color{0};
    glm::vec3 velocity{0};
    float invMass{0};
};

struct BoundingBox{
    alignas(16) glm::vec3 min{ 0 };
    alignas(16) glm::vec3 max{ 1 };
};

struct Sdf{
    Texture texture;
    BoundingBox domain;
};

