#version 460 core

layout(binding = 0) uniform PROJECTION{
    mat4 projection;
};

layout(location = 0) in vec4 pos;
layout(location = 0) smooth out vec2 vUv;

void main(){
    gl_Position = projection * vec4(pos.xy, 0, 1);
    vUv = pos.zw;
}