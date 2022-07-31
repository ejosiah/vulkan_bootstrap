#version 450 core

layout(push_constant) uniform Contants{
    float dt;
    float epsilon;
    float rho;// density;
};

layout(set = 0, binding = 0) uniform sampler2D divergence;
layout(set = 1, binding = 0) uniform sampler2D pressure;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 pressureOut;

float d(vec2 coord) {
    return texture(divergence, fract(coord)).x;
}

float p(vec2 coord) {
    return texture(pressure, fract(coord)).x;
}

void main() {
    vec3 delta = vec3(1.0/textureSize(pressure, 0), 0);
    vec2 dx = 2 * delta.xz;
    vec2 dy = 2 * delta.zy;
    pressureOut = vec4(0.25 * (
    d(uv)
    + p(uv + dx)
    + p(uv - dx)
    + p(uv + dy)
    + p(uv - dy)
    ), 0.0, 0.0, 1.0);
}