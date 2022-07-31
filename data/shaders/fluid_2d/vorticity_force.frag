#version 450 core

layout(set = 0, binding = 0) uniform Globals{
    vec2 dx;
    vec2 dy;
    float dt;
    int ensureBoundaryCondition;
};

#include "common.glsl"

layout(set = 1, binding = 0) uniform sampler2D vorticityField;
layout(set = 2, binding = 0) uniform sampler2D forceField;

layout(push_constant) uniform Constants{
    float csCale;
};

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 force;

float vort(vec2 coord) {
    return texture(vorticityField, st(coord)).x;
}

vec2 accumForce(vec2 coord){
    return texture(forceField, st(coord)).xy;
}

void main(){
    float dudx = (abs(vort(uv + dx)) - abs(vort(uv - dx)))/(2*dx.x);
    float dudy = (abs(vort(uv + dy)) - abs(vort(uv - dy)))/(2*dy.y);

    vec2 n = vec2(dudx, dudy);

    // safe normalize
    float epsilon = 2.4414e-4;
    float magSqr = max(epsilon, dot(n, n));
    n = n * inversesqrt(magSqr);

    float vc = vort(uv);
    vec2 eps = (dx + dy) * csCale;
    force.xy = eps * vc * n * vec2(1, -1) + accumForce(uv);
}