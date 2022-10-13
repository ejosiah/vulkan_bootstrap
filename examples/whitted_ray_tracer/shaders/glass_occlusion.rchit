#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_buffer_reference2 : require

#include "ray_tracing_lang.glsl"
#include "common.glsl"

layout(location = 1) rayPayloadIn ShadowData sd;

void main(){
    sd.isShadowed = true;
    sd.color = vec3(1, 1, 0);
    sd.visibility = 0.7;
}