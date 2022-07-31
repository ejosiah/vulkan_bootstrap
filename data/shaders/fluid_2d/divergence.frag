#version 450 core


layout(set = 0, binding = 0) uniform Globals{
    vec2 dx;
    vec2 dy;
    float dt;
    int ensureBoundaryCondition;
};

#include "common.glsl"

layout(set = 1, binding = 0) uniform sampler2D vectorField;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 divOut;

vec2 u(vec2 coord) {
    return applyBoundaryCondition(coord, texture(vectorField, st(coord)).xy);
}

void main() {
    float dudx = (u(uv + dx).x - u(uv - dx).x)/(2*dx.x);
    float dudy = (u(uv + dy).y - u(uv - dy).y)/(2*dy.y);

    divOut.x = dudx + dudy;
}