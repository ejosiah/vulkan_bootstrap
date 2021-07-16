#pragma once

#include "common.h"
#include "Texture.h"
struct Particle{
    glm::vec4 position{0};
    glm::vec4 color{0};
    glm::vec3 velocity{0};
    float invMass{0};
};

struct ParticleData{
    glm::vec4 color{0};
    glm::vec3 velocity{0};
    float invMass{0};
};


struct BoundingBox{
    alignas(16) glm::vec3 min{ 0 };
    alignas(16) glm::vec3 max{ 1 };
};

inline BoundingBox expand(BoundingBox box, glm::vec3 delta){
    box.min -= delta;
    box.max += delta;
    return box;
}

inline BoundingBox expand(BoundingBox box, float delta){
    box.min -= delta;
    box.max += delta;
    return box;
}

struct Sdf{
    Texture texture;
    BoundingBox domain;
};

