#version 460
#extension GL_EXT_ray_tracing : require

#include "ray_tracing_lang.glsl"

layout(location = 1) rayPayloadIn vec3 visibility;

void main(){
    visibility = vec3(1.0);
}