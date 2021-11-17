#version 460 core

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tanget;
layout(location = 3) in vec3 bitangent;
layout(location = 4) in vec3 color;
layout(location = 5) in vec2 uv;

layout(push_constant) uniform UniformBufferObject{
    mat4 model;
    mat4 view;
    mat4 proj;
};

layout(location = 0) out struct {
    vec3 position;
    vec3 normal;
    vec3 color;
    vec2 uv;
} v_out;

void main(){
    vec4 worldPos = model * position;
    v_out.position = worldPos.xyz;
    v_out.normal = mat3(inverse(transpose(model))) * normal;
    v_out.color = color;
    v_out.uv = uv;
    gl_Position =  proj * view * worldPos;
}