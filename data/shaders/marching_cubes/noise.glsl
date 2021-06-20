#ifndef NOISE_GLSL
#define NOISE_GLSL

#include "..\hash.glsl"

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

float perlin(vec3 p){
    vec3 i = floor(p);
    vec3 f = fract(p);

    vec3 g0 = randomVec(i + vec3(0, 0, 0));
    vec3 g1 = randomVec(i + vec3(1, 0, 0));
    vec3 g2 = randomVec(i + vec3(0, 1, 0));
    vec3 g3 = randomVec(i + vec3(1, 1, 0));
    vec3 g4 = randomVec(i + vec3(0, 0, 1));
    vec3 g5 = randomVec(i + vec3(1, 0, 1));
    vec3 g6 = randomVec(i + vec3(0, 1, 1));
    vec3 g7 = randomVec(i + vec3(1, 1, 1));

    vec3 f0 = f - vec3(0, 0, 0);
    vec3 f1 = f - vec3(1, 0, 0);
    vec3 f2 = f - vec3(0, 1, 0);
    vec3 f3 = f - vec3(1, 1, 0);
    vec3 f4 = f - vec3(0, 0, 1);
    vec3 f5 = f - vec3(1, 0, 1);
    vec3 f6 = f - vec3(0, 1, 1);
    vec3 f7 = f - vec3(1, 1, 1);

    float g0f0 = dot(g0, f0);
    float g1f1 = dot(g1, f1);
    float g2f2 = dot(g2, f2);
    float g3f3 = dot(g3, f3);
    float g4f4 = dot(g4, f4);
    float g5f5 = dot(g5, f5);
    float g6f6 = dot(g6, f6);
    float g7f7 = dot(g7, f7);

    vec3 uv = f * f * f * (10 - 3 * f * (5 - 2 * f));
    float u = uv.x;
    float v = uv.y;
    float w = uv.z;

    return mix(
        mix(mix(g0f0, g1f1, u), mix(g2f2, g3f3, u), v),
        mix(mix(g4f4, g5f5, u), mix(g6f6, g7f7, u), v), w);
}

#endif // NOISE_GLSL