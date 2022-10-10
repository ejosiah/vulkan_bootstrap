#version 460
#extension GL_EXT_ray_tracing : require

#include "raytracing_implicits/implicits.glsl"
#include "raytracing_implicits/common.glsl"
#include "raytracing_implicits/hash.glsl"
#include "ray_tracing_lang.glsl"

#define PATTERN_PARAM 0
#define PATTERN_FUNCTION 0

#define rgb(r, g, b) (vec3(r, g, b) * 0.0039215686274509803921568627451f)

layout(set = 0, binding = 0) uniform accelerationStructure topLevelAS;

layout(set = 1, binding = 0) buffer SPHERES{
    Sphere spheres[];
};


layout(set = 1, binding = 1) buffer PLANES{
    Plane planes[];
};

hitAttribute vec2 attribs;

struct PatternParams{
    vec3 worldPos;
    vec3 normal;
    vec3 color;
    float scale;
};

layout(location = 0) rayPayloadIn vec3 hitValue;
layout(location = PATTERN_PARAM) callableData PatternParams pattern;

void main(){

    vec3 worldPos = gl_WorldRayOrigin + gl_HitT * gl_WorldRayDirection;

    vec3 normal = vec3(0, 1, 0);
    float scale = 1;
    if(gl_HitKind == IMPLICIT_TYPE_SPHERE){
        scale = 2;
        Sphere sphere = spheres[gl_PrimitiveID];
        normal = normalize(sphere.center - worldPos);
    }

    pattern.worldPos = worldPos;
    pattern.normal = normal;
    pattern.scale = scale;

    executeCallable(PATTERN_FUNCTION, PATTERN_PARAM);

    vec3 color = pattern.color;

    vec3 fog = rgb(91, 89, 78);
    float density = 0.1;
    float distFromOrigin = gl_HitT;
    float t = clamp(exp(-density * distFromOrigin), 0, 1);
    hitValue = mix(fog, color, t);
}