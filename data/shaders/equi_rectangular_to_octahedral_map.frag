#version 450 core

#extension GL_GOOGLE_include_directive : enable

#include "octahedral.glsl"

layout(set = 0, binding = 0) uniform sampler2D equirectangularMap;

layout(location = 0) in vec2 vPosition;
layout(location = 0 ) out vec4 fragColor;

const vec2 invAtan = vec2(0.1591, 0.3183);

vec2 sampleSphere(vec3 v){
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main(){
    vec2 uv = sampleSphere(octDecode(vPosition));
    vec3 color = texture(equirectangularMap, uv).rgb;
    color = pow(color, vec3(0.455));
    fragColor = vec4(color, 1.0);
}