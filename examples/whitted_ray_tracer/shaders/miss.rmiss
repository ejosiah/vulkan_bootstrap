#version 460
#extension GL_EXT_ray_tracing : require

#include "ray_tracing_lang.glsl"

#define rgb(r, g, b) (vec3(r, g, b) * 0.0039215686274509803921568627451f)

layout(set = 0, binding = 3) uniform samplerCube skybox;

layout(location = 0) rayPayloadIn vec3 hitValue;

void main(){
    hitValue = texture(skybox, gl_WorldRayDirection).rgb;
}