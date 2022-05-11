#version 450

layout(push_constant) uniform MVP{
    mat4 model;
    mat4 view;
    mat4 projection;
};

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec3 bitangent;
layout(location = 4) in vec3 color;
layout(location = 5) in vec2 uv;

layout(location = 0) out struct {
    vec3 worldPos;
    vec3 viewPos;
    vec3 tangent;
    vec3 bitangent;
    vec3 normal;
    vec2 uv;
} vs_out;

void main(){

    mat3 nMatrix = transpose(inverse(mat3(model)));
    vs_out.tangent = nMatrix * tangent;
    vs_out.bitangent = nMatrix * bitangent;
    vs_out.normal = nMatrix * normal;

    vs_out.worldPos = (model * position).xyz;
    vs_out.viewPos = (inverse(view) *  vec4(0, 0, 0, 1)).xyz;
    vs_out.uv = uv;
    gl_Position = projection * view * model * position;
}