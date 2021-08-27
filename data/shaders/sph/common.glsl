#ifndef COMMON_GLSL
#define COMMON_GLSL

#include "../bounding_box.glsl"

struct Particle{
    vec4 position;
    vec4 color;
    vec3 velocity;
    float invMass;
};

struct ParticleData{
    vec4 color;
    vec3 velocity;
    float density;
    float pressure;
};

#endif // COMMON_GLSL