#version 460

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tanget;
layout(location = 3) in vec3 bitangent;
layout(location = 4) in vec3 color;
layout(location = 5) in vec2 uv;

layout(push_constant) uniform UniformBufferObject{
    mat4 model;
    mat4 view;
    mat4 projection;
};

layout(location = 0) out struct {
    vec3 position;
    vec3 normal;
    vec3 eyes;
} vert_out;

void main(){
    vec4 worldPos = model * position;
    vec3 worldNormal = mat3(model) * normal;

    vert_out.position = worldPos.xyz;
    vert_out.normal = worldNormal;
    vert_out.eyes = (inverse(view) * vec4(0, 0, 0, 1)).xyz;
    gl_Position = projection * view * worldPos;
}