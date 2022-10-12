#version 460
#extension GL_EXT_ray_tracing : require

#include "ray_tracing_lang.glsl"

layout(set = 0, binding = 3) uniform samplerCube skybox;

layout(location = 0) rayPayloadIn vec3 hitValue;

void main(){
    if(gl_WorldRayOrigin.y >= 0){
        hitValue = texture(skybox, gl_WorldRayDirection).rgb;
    }else {
        hitValue = vec3(0);
    }
}