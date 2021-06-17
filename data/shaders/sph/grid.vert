#version 460 core

layout(push_constant) uniform Constants{
    mat4 model;
    mat4 view;
    mat4 projection;
};

layout(location = 0) in vec3 position;

void main(){
    gl_Position = projection * view * model * vec4(position, 1);
}