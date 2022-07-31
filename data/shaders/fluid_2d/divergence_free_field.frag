#version 450 core


layout(set = 0, binding = 0) uniform Globals{
    vec2 dx;
    vec2 dy;
    float dt;
    int ensureBoundaryCondition;
};

#include "common.glsl"

layout(set = 1, binding = 0) uniform sampler2D vectorField;
layout(set = 2, binding = 0) uniform sampler2D pressure;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 velocity_out;

float p(vec2 coord) {
    return texture(pressure, st(coord)).x;
}

vec2 u(vec2 coord) {
    return texture(vectorField, st(coord)).xy;
}

vec2 pg(vec2 coord){
    float dudx = (p(uv + dx) - p(uv - dx))/(2*dx.x);
    float dudy = (p(uv + dy) - p(uv - dy))/(2*dy.y);

    return vec2(dudx, dudy);
}

void main() {
    velocity_out.xy = u(uv) - pg(uv);
}