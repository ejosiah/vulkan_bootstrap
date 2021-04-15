#version 460 core

layout(binding = 0) uniform sampler2D image;
layout(location = 0) in vec2 vUv;
layout(location = 0) out vec4 fragColor;

void main(){
    fragColor = texture(image, vUv);
}