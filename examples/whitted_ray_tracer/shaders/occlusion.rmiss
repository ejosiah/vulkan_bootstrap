#version 460
#extension GL_EXT_ray_tracing : require

#include "ray_tracing_lang.glsl"

layout(location = 1) rayPayloadIn float visibility;

void main(){
    visibility = 1.0;
}