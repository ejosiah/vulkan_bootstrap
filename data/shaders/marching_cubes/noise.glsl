#ifndef NOISE_GLSL
#define NOISE_GLSL

#include "hash.glsl"

vec2 randomVec(vec2 p){
    return 2 * hash22(p) - 1;
}
vec3 randomVec(vec3 p){
    return 2 * hash33(p) - 1;
}

float perlin(vec2 p){
    vec2 i = floor(p);
    vec2 f = fract(p);

    vec2 g0 = randomVec(i + vec2(0, 0));
    vec2 g1 = randomVec(i + vec2(1, 0));
    vec2 g2 = randomVec(i + vec2(0, 1));
    vec2 g3 = randomVec(i + vec2(1, 1));

    vec2 f0 = f - vec2(0, 0);
    vec2 f1 = f - vec2(1, 0);
    vec2 f2 = f - vec2(0, 1);
    vec2 f3 = f - vec2(1, 1);

    float g0f0 = dot(g0, f0);
    float g1f1 = dot(g1, f1);
    float g2f2 = dot(g2, f2);
    float g3f3 = dot(g3, f3);

    vec2 uv = f * f * f * (10 - 3 * f * (5 - 2 * f));
    float u = uv.x;
    float v = uv.y;

    return mix(mix(g0f0, g1f1, u), mix(g2f2, g3f3, u), v);
}

#endif // NOISE_GLSL