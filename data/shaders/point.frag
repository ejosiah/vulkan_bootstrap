#version 460 core

layout(location = 0) in vec4 vColor;
layout(location = 0) out vec4 fragColor;

void main(){
    fragColor = vColor;
}