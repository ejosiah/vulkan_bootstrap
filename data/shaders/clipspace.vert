#version 460 core

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec4 color;

layout(location = 0) out struct {
    vec2 uv;
    vec4 color;
} vOut;

void main(){
    vOut.uv = uv;
    vOut.color = color;
    gl_Position = vec4(position, 0, 1);
}