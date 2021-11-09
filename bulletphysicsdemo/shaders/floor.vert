#version 460 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

layout(push_constant) uniform UniformBufferObject{
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 lightDir;
};

layout(location = 0) out struct {
    vec3 pos;
    vec3  normal;
    vec3 lightDir;
    vec3 eyes;
    vec2 uv;
} v_out;

void main(){
    vec4 worldPos = model * vec4(position, 1.0);
    v_out.pos = worldPos.xyz;
    v_out.normal = mat3(model) * normal;
    v_out.eyes = (inverse(view) * vec4(0, 0, 0, 1)).xyz;
    v_out.lightDir = lightDir;
    v_out.uv = uv;
    gl_Position = proj * view * worldPos;
}