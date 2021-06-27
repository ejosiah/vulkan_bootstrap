#version 460 core

layout(set = 0, binding = 0) uniform MVP{
    mat4 model;
    mat4 view;
    mat4 projection;
};

layout(location = 0) in vec3 position;

void main(){
    gl_Position = projection * view * model * vec4(position, 1);
}