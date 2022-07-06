#version 460 core

layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 uv;

layout(location = 0) out vec2 vUv;

void main(){
    vUv = uv;
    gl_Position = vec4(pos, 0.9, 1);
}