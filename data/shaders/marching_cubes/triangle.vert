#version 460 core

layout(push_constant) uniform Constants{
    mat4 model;
    mat4 view;
    mat4 projection;
};

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

layout (location = 0) out struct {
    vec3 worldPos;
    vec3 normal;
    vec3 eyes;
} vOut;

void main(){
    vOut.worldPos = (model * vec4(position, 1)).xyz;
    vOut.normal = mat3(model) * normal;
    vOut.eyes = (inverse(view) * vec4(0, 0, 0, 1)).xyz;
    gl_Position = projection * view * vec4(vOut.worldPos, 1);
}