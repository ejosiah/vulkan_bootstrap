#version 460

layout(location = 0) in vec4 position;

layout(push_constant) uniform UniformBufferObject{
    mat4 model;
    mat4 view;
    mat4 proj;
};

void main(){
    gl_Position = proj * view * model * position;
}