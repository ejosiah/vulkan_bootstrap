#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

#include "common.glsl"
#include "implicits.glsl"

layout(location = 0) rayPayloadInEXT RayTracePayload payload;

void main(){
    payload.hitValue = payload.bgColor;
}