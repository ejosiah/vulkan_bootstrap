#version 460

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 color;
layout(location = 3) in mat4 local_transform;

layout(push_constant) uniform MVP{
    mat4 model;
    mat4 view;
    mat4 projection;
};

layout(location = 0) out struct{
    vec3 position;
    vec3 normal;
    vec3 color;
} v_out;

void main(){
    mat4 world_transform = model * local_transform;
    vec4 worldPosition = world_transform * position;
    vec3 worldNormal = mat3(world_transform) * normal;

    v_out.position = worldPosition.xyz;
    v_out.normal = worldNormal;
    v_out.color = color;

    gl_Position = projection * view * worldPosition;
}