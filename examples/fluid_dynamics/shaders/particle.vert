#version 450

layout(location = 0) in vec2 position;
layout(location = 0) out vec2 particleId;

void main(){
    gl_PointSize = 1.0;
    particleId = vec2(gl_InstanceIndex);
    gl_Position = vec4(position, -0.1, 1);
}