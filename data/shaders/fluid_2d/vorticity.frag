#version 450 core

#include "calculus.glsl"

layout(set = 0, binding = 0) uniform sampler2D vectorField;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 vort;

vec2 applyBoundaryCondition(vec2 uv, vec2 u){
    if(uv.x <= 0 || uv.x >= 1 || uv.y <= 0 || uv.y >= 1){
        u *= -1;
    }
    return u;
}

vec2 u(vec2 coord) {
    return applyBoundaryCondition(coord, texture(vectorField, coord).xy);
}

void main(){
    vec3 delta = vec3(1.0/textureSize(vectorField, 0), 0);
    vec2 dx = delta.xz;
    vec2 dy = delta.zy;
    float dudx = (u(uv + dx).x - u(uv - dx).x)/(2*dx.x);
    float dudy = (u(uv + dy).y - u(uv - dy).y)/(2*dy.y);

    vort.x = dudy - dudx;
}