#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require

#include "raytracing_implicits/implicits.glsl"
#include "raytracing_implicits/common.glsl"
#include "raytracing_implicits/hash.glsl"
#include "ray_tracing_lang.glsl"
#include "pbr/common.glsl"
#include "common.glsl"

#define PATTERN_PARAM 0
#define PATTERN_FUNCTION 0

#define rgb(r, g, b) (vec3(r, g, b) * 0.0039215686274509803921568627451f)

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


hitAttribute vec2 attribs;

layout(location = 0) rayPayloadIn RaytraceData rtData;
layout(location = 1) rayPayload ShadowData sd;

layout(location = PATTERN_PARAM) callableData PatternParams pattern;


vec3 checkerboardPattern(vec3 pos, vec3 normal, float scale){
    pattern.worldPos = pos;
    pattern.normal = normal;
    pattern.scale = scale;

    executeCallable(PATTERN_FUNCTION, PATTERN_PARAM);
    return pattern.color;
}

const float preventDivideByZero = 0.0001;


void main(){
    if(rtData.depth >= 5) return;
    rtData.depth += 1;
    vec3 worldPos = gl_WorldRayOrigin + gl_HitT * gl_WorldRayDirection;

    vec3 normal = vec3(0);

    float scale = 1;
    int matOffset = 0;
    Material material;
    vec3 albedo = vec3(0);
    LightSource light = LightSource(vec3(0, 10, 10), vec3(1000));

    if(gl_HitKind == IMPLICIT_TYPE_SPHERE){
        Sphere sphere = spheres.at[gl_PrimitiveID];
        normal = normalize(worldPos - sphere.center);
        material = materials.at[gl_PrimitiveID];
        albedo = gl_PrimitiveID == 0 ? checkerboardPattern(worldPos, normal, 1) :  material.albedo;
    }else{
        material = materials.at[gl_PrimitiveID];
        albedo = checkerboardPattern(worldPos, normal, scale);
        normal = planes.at[gl_PrimitiveID].normal;
        bool viewIsBelowPlane = dot(normal, -gl_WorldRayDirection) < 0.0001;
        if(viewIsBelowPlane){
            normal *= -1;
        }
    }

    float metalness = material.metalness;
    float roughness = material.roughness;

    vec3 viewPos = gl_WorldRayOrigin;
    vec3 lightPos = light.locaiton;
    vec3 lightDir = lightPos - worldPos;
    vec3 viewDir = viewPos - worldPos;

    vec3 L = normalize(lightDir);
    vec3 E = normalize(viewDir);
    vec3 H = normalize(E + L);
    vec3 N = normal;
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metalness);

    float dist = length(lightDir);
    float attenuation = 1/(dist * dist);

    // Cook-Torrance BRDF
    float NDF = distributionGGX(N, H, roughness);
    float G = geometrySmith(N, E, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H,E), 0), F0);

    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(N, E), 0.0) * max(dot(N, L), 0.0) + preventDivideByZero;
    vec3 specular = numerator / denominator;

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metalness;

    vec3 reflectColor = vec3(1);
    if(length(kS) > 0.001){


        vec3 direction = reflect(gl_WorldRayDirection, normal);
        direction = normalize(direction);

        traceRay(topLevelAS, gl_RayFlagsOpaque, 0xFF, 0, 0, 0, worldPos, HIT_MARGIN, direction, 10000, 0);
        reflectColor = rtData.hitValue;
    }

    vec3 ambiance = albedo * 0.2;
    vec3 diffuse = albedo * max(0, dot(N, L));
    vec3 pSpecular = vec3(1) * pow(max(0, dot(N, H)), 250);
    vec3 radiance = light.intensity * attenuation;

    uint flags =  gl_RayFlagsOpaque;
    sd = ShadowData(vec3(0), 0.3, true);
    float NdotL = max(0, dot(N, L));

    if(NdotL > 0){
        traceRay(topLevelAS, gl_RayFlagsNoOpaque, 0xFF, 3, 0, 1, worldPos, HIT_MARGIN, L, dist, 1);
    }


    vec3 irradience = sd.visibility * (kD * albedo / PI + specular) * radiance * NdotL;
    irradience += reflectColor * kS;
//    vec3 irradience = ambiance + radiance * (diffuse + pSpecular);
    rtData.hitValue = irradience/(irradience + 1);

    vec3 fog = rgb(91, 89, 78);
    float density = 0.05;
    float distFromOrigin = gl_HitT;
    float t = clamp(exp(-density * distFromOrigin), 0, 1);
    rtData.hitValue = mix(fog, rtData.hitValue, t);
}