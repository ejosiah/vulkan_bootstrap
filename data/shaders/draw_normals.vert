#version 460 core

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec4 color;
layout(location = 3) in vec2 uv;
layout(location = 4) in vec3 tangent;
layout(location = 5) in vec3 bitangent;


layout(location = 0) out struct {
    vec4 color;
    vec3 normal;
} vOut;

void main(){
    vOut.color = color;
    vOut.normal = normal;
    gl_PointSize = 1.0;
    gl_Position = position;
}