#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_EXT_buffer_reference2 : require

#include "ray_tracing_lang.glsl"
#include "raytracing_implicits/implicits.glsl"
#include "raytracing_implicits/common.glsl"
#include "raytracing_implicits/hash.glsl"
#include "common.glsl"

layout(set = 0, binding = 0) uniform accelerationStructure topLevelAS;

layout(buffer_reference, buffer_reference_align=8) buffer SphereBuffer{
    Sphere at[];
};

layout(buffer_reference, buffer_reference_align=8) buffer PlaneBuffer{
    Plane at[];
};

layout(buffer_reference, buffer_reference_align=8) buffer MaterialBuffer{
    Material at[];
};

layout(shaderRecord, std430) buffer SBT {
    SphereBuffer spheres;
    PlaneBuffer planes;
    MaterialBuffer materials;
};

layout(location = 0) rayPayloadIn RaytraceData rtData;

hitAttribute vec2 attribs;


void main(){
    if(rtData.depth >= 5) return;
    rtData.depth += 1;
    LightSource light = LightSource(vec3(0, 10, 0), vec3(10));

    vec3 normal = vec3(0);
    vec3 hitPoint = gl_WorldRayOrigin + gl_HitT * gl_WorldRayDirection;


    if(gl_HitKind == IMPLICIT_TYPE_SPHERE){
        Sphere sphere = spheres.at[gl_PrimitiveID];
        normal = normalize(hitPoint - sphere.center);
    }else{
        normal = planes.at[gl_PrimitiveID].normal;
    }

    vec3 direction = reflect(gl_WorldRayDirection, normal);
    direction = normalize(direction);

    traceRay(topLevelAS, gl_RayFlagsOpaque, 0xFF, 0, 1, 0, hitPoint, HIT_MARGIN, direction, 10000, 0);
    rtData.hitValue *= vec3(0.9);
}