#version 460 core

layout(location = 0) in vec2 position;

layout(location = 0) out vec2 vPosition;

void main(){
    vPosition = position;
    gl_Position = vec4(position, 0, 1);
}