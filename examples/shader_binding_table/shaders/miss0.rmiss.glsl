#version 460
#extension GL_EXT_ray_tracing : enable

#include "ray_tracing_lang.glsl"

#define rgb(r, g, b) (vec3(r, g, b) * 0.0039215686274509803921568627451f)

layout(location = 0) rayPayloadInEXT vec3 hitValue;

void main(){
    hitValue = mix(rgb(184, 224, 242), rgb(65, 186, 242), gl_WorldRayDirection.y);
}