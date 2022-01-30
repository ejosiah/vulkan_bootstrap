#version 460

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tanget;
layout(location = 3) in vec3 bitangent;
layout(location = 4) in vec3 color;
layout(location = 5) in vec2 uv;

layout(push_constant) uniform MVP{
    mat4 model;
    mat4 view;
    mat4 projection;
};

layout(location = 0) out struct {
    vec3 viewPos;
    vec3 viewNormal;
    vec3 color;
} v_out;

void main(){
    mat4 MV = view * model;
    vec4 viewPos = MV * position;
    vec3 viewNormal = mat3(MV) * normal;
    v_out.viewPos = viewPos.xyz;
    v_out.viewNormal = viewNormal;
    v_out.color = color;

    gl_Position = projection * viewPos;
}