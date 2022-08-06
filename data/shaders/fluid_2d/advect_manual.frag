#version 450 core

layout(set = 0, binding = 0) uniform Globals{
    vec2 dx;
    vec2 dy;
    float dt;
    int ensureBoundaryCondition;
    int useHermite;
};

#include "common.glsl"

layout(set = 1, binding = 0) uniform sampler2D vectorField;
layout(set = 2, binding = 0) uniform texture2D quantity;
layout(set = 3, binding = 0) uniform sampler linerSampler;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 quantityOut;

void main(){
    vec2 u = texture(vectorField, uv).xy;

    vec2 p = st(uv - dt * u)/(dx + dy);

    vec2 p0 = floor(p - 0.5) + 0.5;
    vec2 f = p - p0;
    vec2 t = bool(useHermite) ? f * f * (3 - 2 * f) : f;
    p0  = p0 * (dx + dy);
    vec2 p1 = p0 + dx;
    vec2 p2 = p0 + dy;
    vec2 p3 = p1 + dy;

    vec4 q0 = texture(sampler2D(quantity, linerSampler), p0);
    vec4 q1 = texture(sampler2D(quantity, linerSampler), p1);
    vec4 q2 = texture(sampler2D(quantity, linerSampler), p2);
    vec4 q3 = texture(sampler2D(quantity, linerSampler), p3);

    quantityOut = mix(mix(q0, q1, t.x), mix(q2, q3, t.x), t.y);
}