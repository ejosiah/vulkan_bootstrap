#version 450 core

#include "calculus.glsl"

layout(set = 0, binding = 0) uniform sampler2D vectorField;

layout(push_constant) uniform Contants{
    float dt;
    float epsilon;
    float rho;// density;
};

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 divOut;

vec2 applyBoundaryCondition(vec2 uv, vec2 u){
    if(uv.x <= 0 || u.x >= 1){
        u.x *= -1;
    }
    if(u.y <= 0 || u.y >= 1){
        u.y *= -1;
    }
    return u;
}

vec2 u(vec2 coord) {
    return applyBoundaryCondition(coord, texture(vectorField, coord).xy);
}

void main() {
    vec3 delta = vec3(1.0/textureSize(vectorField, 0), 0);
    vec2 dx = delta.xz;
    vec2 dy = delta.zy;
    float alpha = (-2.0 * epsilon * rho / dt);
    float dudx = (u(uv + dx).x - u(uv - dx).x)/(2*dx.x);
    float dudy = (u(uv + dy).y - u(uv - dy).y)/(2*dy.y);

    divOut.x = dudx + dudy;
//    divOut.xy = uv + dy;
}