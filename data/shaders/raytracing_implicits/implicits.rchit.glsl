#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

#include "implicits.glsl"
#include "common.glsl"
#include "hash.glsl"

layout(set = 1, binding = 0) buffer SPHERES{
    Sphere spheres[];
};

layout(set = 1, binding = 1) buffer PLANES{
    Plane planes[];
};

hitAttributeEXT vec2 attribs;

layout(location = 0) rayPayloadInEXT RayTracePayload payload;

void main(){
    vec3 p = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    vec3 L = -gl_WorldRayDirectionEXT;
    if(gl_HitKindEXT == IMPLICIT_TYPE_SPHERE){
        Sphere sphere = spheres[gl_PrimitiveID];
        vec3 N = normalize(p - sphere.center);

        payload.hitValue = hash33(sphere.center) * max(0, dot(N, L));
    }else if(gl_HitKindEXT == IMPLICIT_TYPE_PLANE){
        Plane plane = planes[gl_PrimitiveID];
        vec3 N = plane.normal;
        payload.hitValue = checkerboard(attribs) * max(0, dot(N, L));
    }
   // payload.hitValue = vec3(1, 0, 0);
}