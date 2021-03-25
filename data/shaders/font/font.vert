#version 460 core

layout(binding = 0) uniform PROJECTION{
    mat4 projection;
};

layout(location = 0) in vec4 pos;
layout(location = 0) smooth out vec2 vUv;

void main(){
    vec2 nPos = pos.xy;
    gl_Position = projection * vec4(nPos, 0, 1);
    vUv = pos.zw;
}