#version 460
#extension GL_EXT_ray_tracing : enable

#define rgb(r, g, b) (vec3(r, g, b) * 0.0039215686274509803921568627451f)

layout(location = 0) rayPayloadInEXT vec3 hitValue;

void main()
{
    hitValue = mix(vec3(1), rgb(138, 175, 235), gl_WorldRayOriginEXT.y);
}