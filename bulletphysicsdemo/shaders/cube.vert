#version 460 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 color;
layout(location = 3) in mat4 local_transform;

layout(push_constant) uniform UniformBufferObject{
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 lightDir;
};

layout(location = 0) out struct {
    vec3 pos;
    vec3  normal;
    vec3 color;
    vec3 lightDir;
    vec3 eyes;
} v_out;

void main(){
    mat4 world_transform = model * local_transform;
    vec4 worldPos = world_transform * vec4(position, 1.0);
    v_out.color = color;
    v_out.pos = worldPos.xyz;
    v_out.normal = mat3(world_transform) * normal;
    v_out.eyes = (inverse(view) * vec4(0, 0, 0, 1)).xyz;
    v_out.lightDir = lightDir;

    gl_PointSize = 2.0;
    gl_Position = proj * view * worldPos;
}