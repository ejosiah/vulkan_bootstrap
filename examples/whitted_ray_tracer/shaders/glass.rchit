#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_EXT_buffer_reference2 : require

#include "ray_tracing_lang.glsl"
#include "raytracing_implicits/implicits.glsl"
#include "raytracing_implicits/common.glsl"
#include "raytracing_implicits/hash.glsl"

#define HIT_MARGIN 0.0001

struct LightSource{
    vec3 locaiton;
    vec3 intensity;
};

struct Material{
    vec3 albedo;
    vec2 padding;
    float metalness;
    float roughness;
};

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

layout(location = 0) rayPayloadIn vec3 hitValue;

hitAttribute vec2 attribs;

const float Air = 1.0003;
const float Medium = 1.52;

void swap(inout float a, inout float b){
    float temp = a;
    a = b;
    b = temp;
}

float fresnelDielectric(float cosThetaI, float etaI, float etaT) {
    cosThetaI = clamp(cosThetaI, -1.0, 1.0);
    // Potentially swap indices of refraction
    bool entering = cosThetaI > 0.f;
    if (!entering) {
        swap(etaI, etaT);
        cosThetaI = abs(cosThetaI);
    }

    // Compute _cosThetaT_ using Snell's law
    float sinThetaI = sqrt(max(0, 1 - cosThetaI * cosThetaI));
    float sinThetaT = etaI / etaT * sinThetaI;

    // Handle total internal reflection
    if (sinThetaT >= 1) {
        return 1;
    }
    float cosThetaT = sqrt(max(0, 1 - sinThetaT * sinThetaT));
    float Rparl = ((etaT * cosThetaI) - (etaI * cosThetaT)) /
    ((etaT * cosThetaI) + (etaI * cosThetaT));
    float Rperp = ((etaI * cosThetaI) - (etaT * cosThetaT)) /
    ((etaI * cosThetaI) + (etaT * cosThetaT));
    return (Rparl * Rparl + Rperp * Rperp) / 2;
}

float fresnel(float cosTheta, float etaI, float etaT) {
    //return fresnelSchlick(cosTheta, f0(etaI, etaT));
    return fresnelDielectric(cosTheta, etaI, etaT);
}

void main(){
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
    float n1 = Medium;
    vec3 I = normalize(gl_WorldRayDirection);
    vec3 N = normal;
    float cos0 = dot(-I, N);
    if(cos0 < 0){
        swap(n0, n1);
        N *= -1;
    }

    float eta = n0/n1;
    vec3 direction = refract(I, N, eta);

    traceRay(topLevelAS, gl_RayFlagsOpaque, 0xFF, 0, 0, 0, hitPoint, HIT_MARGIN, direction, 10000, 0);
}