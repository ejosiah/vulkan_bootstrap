#version 450 core

layout(set = 0, binding = 0) uniform sampler2D solution;
layout(set = 1, binding = 0) uniform sampler2D unknown;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 x;

layout(push_constant) uniform Constants {
    float alpha;
    float rBeta;
};

vec4 b(vec2 coord){
    return texture(solution, fract(coord));
}

vec4 x0(vec2 coord){
    return texture(unknown, fract(coord));
}

void main(){
    vec2 size = textureSize(unknown, 0);
    vec3 delta = vec3(1.0/size, 0);
    vec2 dx = delta.xz;
    vec2 dy = delta.zy;

    x = (x0(uv + dx) + x0(uv - dx) + x0(uv + dy) + x0(uv - dy) + alpha * b(uv)) * rBeta;
}