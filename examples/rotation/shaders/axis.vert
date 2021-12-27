#version 460

layout(location = 0) in vec4 position;
layout(location = 1) in vec4 color;

layout(push_constant) uniform UniformBufferObject{
    mat4 model;
    mat4 view;
    mat4 proj;
};

layout(location = 0) out vec4 vColor;

void main(){
    vColor = color;
    gl_Position = proj * view * model * position;
}