#pragma once

#include "common.h"
#include "Texture.h"

constexpr int PARTICLES_PER_INVOCATION = 1024;

constexpr int workGroupSize(const int NumWorkGroups, const int invocationsPerWorkGroup = PARTICLES_PER_INVOCATION){
    return (NumWorkGroups - 1)/invocationsPerWorkGroup + 1;
}

struct Particle{
    glm::vec4 position{0};
    glm::vec4 color{0};
    glm::vec3 velocity{0};
    float invMass{0};
};

struct ParticleData{
    glm::vec4 color{0};
    glm::vec3 velocity{0};
    float density{0};
    float pressure{0};
};

struct BoundingBox{
    glm::vec3 min{ 0 };
    glm::vec3 max{ 1 };
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

inline glm::vec3 diagonal(BoundingBox box){
    return box.max - box.min;
}

inline float height(BoundingBox box){
    return diagonal(box).y;
}

inline float width(BoundingBox box){
    return diagonal(box).z;
}
inline float depth(BoundingBox box){
    return diagonal(box).z;
}

struct Sdf{
    Texture texture;
    BoundingBox domain;
};

struct BoxSurface{
    BoundingBox bounds;
    int flipNormal{0};
};