#version 450 core

layout(set = 0, binding = 0) uniform Globals{
    vec2 dx;
    vec2 dy;
    float dt;
    int ensureBoundaryCondition;
};

#include "common.glsl"

layout(set = 1, binding = 0) uniform sampler2D vectorField;
layout(set = 2, binding = 0) uniform texture2D quantity;
layout(set = 3, binding = 0) uniform sampler linerSampler;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 quantityOut;

void main(){
    vec2 u = texture(vectorField, uv).xy;

    vec2 p = st(uv - dt * u);
    quantityOut = texture(sampler2D(quantity, linerSampler), p);
}