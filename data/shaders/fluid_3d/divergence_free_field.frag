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
layout(set = 2, binding = 0) uniform sampler3D pressure;

layout(location = 0) in vec3 uv;
layout(location = 0) out vec4 velocity_out;

float p(vec3 coord) {
    return texture(pressure, st(coord)).x;
}

vec3 u(vec3 coord) {
    return texture(vectorField, st(coord)).xyz;
}

vec3 pg(){
    float dudx = (p(uv + dx) - p(uv - dx))/(2*dx.x);
    float dudy = (p(uv + dy) - p(uv - dy))/(2*dy.y);
    float dudz = (p(uv + dz) - p(uv - dz))/(2*dy.z);

    return vec3(dudx, dudy, dudz);
}

void main() {
    velocity_out.xyz = u(uv) - pg();
}