#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

#include "implicits.glsl"
#include "common.glsl"
#include "hash.glsl"
layout(set = 0, binding = 0) uniform accelerationStructureEXT topLevelAS;

layout(set = 1, binding = 0) buffer SPHERES{
    Sphere spheres[];
};

layout(set = 1, binding = 1) buffer PLANES{
    Plane planes[];
};

hitAttributeEXT vec2 attribs;

layout(location = 0) rayPayloadInEXT RayTracePayload payload;
layout(location = 1) rayPayloadEXT bool inShadow;

void main(){
    vec3 p = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    vec3 lightPos = vec3(0, 5, -5);
    vec3 lightDir = lightPos - p;
    vec3 L = normalize(lightDir);
    vec3 diffuse = vec3(0);
    vec3 N = vec3(0);
    float shine = 50;
    vec3 sColor = vec3(1);
    if(gl_HitKindEXT == IMPLICIT_TYPE_SPHERE){
        Sphere sphere = spheres[gl_PrimitiveID];
        N = normalize(p - sphere.center);

        diffuse = hash33(sphere.center) * max(0, dot(N, L));
    }else if(gl_HitKindEXT == IMPLICIT_TYPE_PLANE){
        sColor = vec3(0.2);
        Plane plane = planes[gl_PrimitiveID];
        N = plane.normal;
        diffuse = checkerboard(attribs) * max(0, dot(N, L));
    }
    float attenumation = 1.0;
    vec3 specular = vec3(0);
    if(dot(N, L) > 0){
        uint flags = gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT | gl_RayFlagsTerminateOnFirstHitEXT;
        float tmin = 0.01;
        float tmax = length(lightDir);
        inShadow = true;
        traceRayEXT(topLevelAS, flags, 0xFF, 0, 0, 1, p, tmin,  L, tmax, 1);
        if(inShadow) attenumation = 0.3;
    }

    if(!inShadow){
        vec3 E = normalize(gl_WorldRayOriginEXT - p);
        vec3 H = normalize(E + L);
        float shine = 50;
        specular = max(0, pow(dot(N, H), shine)) * sColor;
    }

    payload.hitValue = payload.bgColor * attenumation * (diffuse + specular);
}