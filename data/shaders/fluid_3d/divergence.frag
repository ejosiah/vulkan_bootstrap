#version 450 core


layout(set = 0, binding = 0) uniform Globals{
    vec3 dx;
    vec3 dy;
    vec3 dz;
    float dt;
    int ensureBoundaryCondition;
};

#include "common.glsl"

layout(set = 1, binding = 0) uniform sampler3D vectorField;

layout(location = 0) in vec3 uv;
layout(location = 0) out vec4 divOut;

vec3 u(vec3 coord) {
    return applyBoundaryCondition(coord, texture(vectorField, st(coord)).xyz);
}

void main() {
    float dudx = (u(uv + dx).x - u(uv - dx).x)/(2*dx.x);
    float dudy = (u(uv + dy).y - u(uv - dy).y)/(2*dy.y);
    float dudz = (u(uv + dz).z - u(uv - dz).y)/(2*dz.z);

    divOut.x = dudx + dudy + dudz;
}