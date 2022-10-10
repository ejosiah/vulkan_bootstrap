#version 460
#extension GL_EXT_ray_tracing : require

#include "raytracing_implicits/implicits.glsl"
#include "raytracing_implicits/common.glsl"
#include "raytracing_implicits/hash.glsl"
#include "ray_tracing_lang.glsl"

#define rgb(r, g, b) (vec3(r, g, b) * 0.0039215686274509803921568627451f)

layout(set = 0, binding = 0) uniform accelerationStructure topLevelAS;

layout(set = 1, binding = 0) buffer SPHERES{
    Sphere spheres[];
};


layout(set = 1, binding = 1) buffer PLANES{
    Plane planes[];
};

hitAttribute vec2 attribs;

struct PatternParams{
    vec3 worldPos;
    vec3 color;
};

layout(location = 0) rayPayloadIn vec3 hitValue;
layout(location = 0) callableData PatternParams pattern;

vec3 GetColorFromPositionAndNormal( in vec3 worldPosition, in vec3 normal, float scale) {
    const float pi = 3.141519;

    vec3 scaledPos = 2 * worldPosition.xyz * pi * 2.0;
    vec3 scaledPos2 = 2 * worldPosition.xyz * pi * 2.0 / 10.0 + vec3( pi / 4.0 );
    scaledPos *= scale;
    scaledPos *= scale;
    float s = cos( scaledPos2.x ) * cos( scaledPos2.y ) * cos( scaledPos2.z );  // [-1,1] range
    float t = cos( scaledPos.x ) * cos( scaledPos.y ) * cos( scaledPos.z );     // [-1,1] range


    t = ceil( t * 0.9 );
    s = ( ceil( s * 0.9 ) + 3.0 ) * 0.25;
    vec3 colorB = vec3( 0.85, 0.85, 0.85 );
    vec3 colorA = vec3( 1, 1, 1 );
    vec3 finalColor = mix( colorA, colorB, t ) * s;

    return vec3(0.8) * finalColor;
}

void main(){

    vec3 worldPos = gl_WorldRayOrigin + gl_HitT * gl_WorldRayDirection;

    pattern.worldPos = worldPos;
    executeCallable(0, 0);
    vec3 color = pattern.color;
//    vec3 normal = vec3(0, 1, 0);
//    float scale = 1;
//    if(gl_HitKind == IMPLICIT_TYPE_SPHERE){
//        scale = 2.5;
//        Sphere sphere = spheres[gl_PrimitiveID];
//        normal = normalize(sphere.center - worldPos);
//    }
//    vec3 color = GetColorFromPositionAndNormal(worldPos, normal, scale);

    vec3 fog = rgb(91, 89, 78);
    float density = 0.1;
    float distFromOrigin = gl_HitT;
    float t = clamp(exp(-density * distFromOrigin), 0, 1);
    hitValue = mix(fog, color, t);
}