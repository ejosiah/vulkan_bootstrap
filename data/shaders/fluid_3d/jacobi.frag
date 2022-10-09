#version 450 core


layout(set = 0, binding = 0) uniform Globals{
    vec3 dx;
    vec3 dy;
    vec3 dz;
    float dt;
    int ensureBoundaryCondition;
};

#include "common.glsl"

layout(set = 1, binding = 0) uniform sampler3D solution;
layout(set = 2, binding = 0) uniform sampler3D unknown;

layout(location = 0) in vec3 uv;
layout(location = 0) out vec4 x;

layout(push_constant) uniform Constants {
    float alpha;
    float rBeta;
    int isVectorField;
};

vec4 b(vec3 coord){
    vec4 _b = texture(solution, st(coord));
    if(isVectorField == 1){
        return applyBoundaryCondition(coord, _b);
    }
    return _b;
}

vec4 x0(vec3 coord){
    vec4 _x0 = texture(unknown, st(coord));
    if(isVectorField == 1){
        return applyBoundaryCondition(coord, _x0);
    }
    return _x0;
}

void main(){
    float dxdx = dx.x * dx.x;
    float dydy = dy.y * dy.y;
    float dzdz = dz.z * dz.z;

    vec4 xx = (x0(uv + dx) + x0(uv - dx)) * dydy * dzdz;
    vec4 xy = (x0(uv + dy) + x0(uv - dy)) * dxdx * dzdz;
    vec4 xz = (x0(uv + dz) + x0(uv - dz)) * dxdx * dydy;

    x = (xx + xy + xz + alpha * b(uv)) * rBeta;
}