#version 460 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 color;
layout(location = 3) in vec2 uv;
layout(location = 4) in mat4 xform;

layout(set = 0, binding = 0) uniform UniformBufferObject{
    mat4 model;
    mat4 view;
    mat4 proj;
};


layout(location = 0) out vec3 vColor;
layout(location = 1) out vec2 vUv;


void main(){
    gl_Position = proj * view * model * xform * vec4(position, 1.0);
    vColor = vec3(1);
    vUv = uv;
}