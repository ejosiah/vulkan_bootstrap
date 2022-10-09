#version 450 core

layout(set = 0, binding = 0) uniform Globals{
    vec2 dx;
    vec2 dy;
    vec3 dz;
    float dt;
    int ensureBoundaryCondition;
};

#include "common.glsl"

layout(set = 1, binding = 0) uniform sampler3D vectorField;
layout(set = 2, binding = 0) uniform texture3D quantity;
layout(set = 3, binding = 0) uniform sampler linerSampler;

layout(location = 0) in vec3 uv;
layout(location = 0) out vec4 quantityOut;

void main(){
    vec3 u = texture(vectorField, uv).xyz;

    vec3 p = st(uv - dt * u);
    quantityOut = texture(sampler3D(quantity, linerSampler), p);
}