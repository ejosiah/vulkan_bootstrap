#version 460 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tanget;
layout(location = 3) in vec3 bitangent;
layout(location = 4) in vec3 color;
layout(location = 5) in vec2 uv;

void main(){
    gl_Position = vec4(position, 1.0);

}