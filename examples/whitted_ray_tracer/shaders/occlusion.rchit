#version 460
#extension GL_EXT_ray_tracing : require
#include "ray_tracing_lang.glsl"
#include "common.glsl"

layout(location = 1) rayPayloadIn ShadowData sd;

void main(){
    sd.isShadowed = true;
    sd.color = vec3(0, 0, 1);
    sd.visibility = 0.3;
}