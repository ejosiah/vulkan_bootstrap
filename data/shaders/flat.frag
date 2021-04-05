#version 460 core

layout(location = 0) in vec3 vColor;
layout(location = 1) in vec2 vUv;

layout(location = 0) out vec4 fracColor;

void main(){
    fracColor = vec4(vColor, 1);
}