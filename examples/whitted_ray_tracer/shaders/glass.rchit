#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_EXT_buffer_reference2 : require

#include "ray_tracing_lang.glsl"
#include "raytracing_implicits/implicits.glsl"
#include "raytracing_implicits/common.glsl"
#include "raytracing_implicits/hash.glsl"
#include "common.glsl"

#define FRESNEL_CALLABLE_PARAMS_INDEX 1
#define FRESNEL_CALLABLE_FUNC_INDEX 1

layout(set = 0, binding = 0) uniform accelerationStructure topLevelAS;

layout(buffer_reference, buffer_reference_align=8) buffer SphereBuffer {
    Sphere at[];
};

layout(buffer_reference, buffer_reference_align=8) buffer PlaneBuffer {
    Plane at[];
};

layout(buffer_reference, buffer_reference_align=8) buffer GlassMatBuffer {
    GlassMaterial at[];
};

layout(shaderRecord, std430) buffer SBT {
    SphereBuffer spheres;
    PlaneBuffer planes;
    GlassMatBuffer materials;
};

layout(location = 0) rayPayloadIn RaytraceData rtData;
layout(location = 1) callableData FresnelParams fParams;

hitAttribute vec2 attribs;

const float Air = 1.0003;

float fresnel(float cosTheta, float etaI, float etaT) {
    fParams = FresnelParams(cosTheta, etaI, etaT, 0);
    executeCallable(FRESNEL_CALLABLE_FUNC_INDEX, FRESNEL_CALLABLE_PARAMS_INDEX);
    return fParams.result;
}

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

    float n0 = Air;
    float n1 = materials.at[gl_PrimitiveID].ior;
    vec3 I = normalize(gl_WorldRayDirection);
    vec3 N = normal;
    float cos0 = dot(-I, N);

    float ks = fresnel(cos0, n0, n1);
    float kt = 1 - ks;


    if (cos0 < 0){
        swap(n0, n1);
        N *= -1;
    }

    vec3 refrctColor = vec3(1);
    if(kt > 0.0001){
        float eta = n0/n1;
        vec3 direction = refract(I, N, eta);

        traceRay(topLevelAS, gl_RayFlagsOpaque, 0xFF, 0, 1, 0, hitPoint, HIT_MARGIN, direction, 10000, 0);
        refrctColor = rtData.hitValue;
    }

    vec3 reflectColor = vec3(1);
    if(ks > 0.0001){
        vec3 direction = reflect(I, N);
        traceRay(topLevelAS, gl_RayFlagsOpaque, 0xFF, 0, 1, 0, hitPoint, HIT_MARGIN, direction, 10000, 0);
        reflectColor = rtData.hitValue;
    }

    vec3 albedo = materials.at[gl_PrimitiveID].albedo;
    rtData.hitValue = albedo * (ks * reflectColor + kt * refrctColor);
}