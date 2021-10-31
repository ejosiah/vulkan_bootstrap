#version 460 core

layout(push_constant) uniform MVP {
    mat4 model;
    mat4 view;
    mat4 projection;
    vec3 color;
};

layout(location = 0) in vec4 position;
layout(location = 0) out vec3 vColor;

void main(){
    vColor = color;
    gl_Position = projection * view * model * position;
}