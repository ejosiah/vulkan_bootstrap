#version 450

layout(location = 0) in vec2 position;
layout(location = 1) in float orientation;

layout(location = 0) out float vOrientation;

void main(){
    vOrientation = orientation;
    gl_Position = vec4(position, 0, 1);
}