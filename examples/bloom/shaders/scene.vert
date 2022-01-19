#version 460

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec3 bitangent;
layout(location = 4) in vec4 color;
layout(location = 5) in vec2 uv;


layout(push_constant) uniform MVP{
    mat4 model;
    mat4 view;
    mat4 projection;
};

layout(location = 0) out struct {
    vec3 posotion;
    vec3 normal;
    vec2 uv;
} v_out;

void main(){
    vec4 worldPos = model * position;
    vec3 worldNormal = mat3(model) * normal;

    v_out.posotion = worldPos.xyz;
    v_out.normal = worldNormal;
    v_out.uv = uv;
    gl_Position = projection * view * worldPos;
}