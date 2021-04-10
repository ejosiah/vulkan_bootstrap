#version 460 core

layout(binding = 0) uniform sampler2D albedoMap;
layout(binding = 1) uniform sampler2D lightMap;

layout(location = 0) in vec2 vUv;

layout(location = 0) out vec4 fragColor;

void main(){
    fragColor = texture(albedoMap, vUv * 8) * texture(lightMap, vUv);
}