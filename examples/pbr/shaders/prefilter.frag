#version 450 core

#extension GL_GOOGLE_include_directive : enable
#include "octahedral.glsl"

#define PI 3.1415926535897932384626
#define TWO_PI (PI * 2.0)
#define HALF_PI (PI * 0.5)

layout(set = 0, binding = 0) uniform sampler2D environmentMap;

layout(location = 0) in vec2 vPosition;
layout(location = 0 ) out vec4 fragColor;


void main(){
    vec3 normal = octDecode(vPosition);

    vec3 irradiance = vec3(0);

    vec3 up = vec3(0, 1, 0);
    vec3 right = normalize(cross(up, normal));
    up  = normalize(cross(normal, right));

    float sampleDelta = 0.025;
    float nrSamples = 0.0;

    for(float phi = 0.0; phi < TWO_PI; phi += sampleDelta){
        for(float theta = 0.0; theta < HALF_PI; theta += sampleDelta){
            // spherical to cartesain (in tangent space)
            vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));

            // tangent space to world space
            vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * normal;
            vec2 uv = octEncode(normalize(sampleVec)) * 0.5 + 0.5;
            irradiance += texture(environmentMap, uv).rgb * cos(theta) * sin(theta);
            nrSamples++;
        }
    }
    irradiance = PI * irradiance * (1/float(nrSamples));

    fragColor = vec4(irradiance, 1);
}