#version 460 core

layout(push_constant) uniform UniformBufferObject{
    mat4 model;
    mat4 view;
    mat4 proj;
};

layout(location = 0) in vec3 position;

void main(){
    gl_Position = proj * view * model * vec4(position, 1);
}