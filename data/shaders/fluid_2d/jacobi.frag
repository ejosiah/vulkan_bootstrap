#version 450 core


layout(set = 0, binding = 0) uniform Globals{
    vec2 dx;
    vec2 dy;
    float dt;
    int ensureBoundaryCondition;
};

#include "common.glsl"

layout(set = 1, binding = 0) uniform sampler2D solution;
layout(set = 2, binding = 0) uniform sampler2D unknown;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 x;

layout(push_constant) uniform Constants {
    float alpha;
    float rBeta;
    int isVectorField;
};

vec4 b(vec2 coord){
    vec4 _b = texture(solution, st(coord));
    if(isVectorField == 1){
        return applyBoundaryCondition(coord, _b);
    }
    return _b;
}

vec4 x0(vec2 coord){
    vec4 _x0 = texture(unknown, st(coord));
    if(isVectorField == 1){
        return applyBoundaryCondition(coord, _x0);
    }
    return _x0;
}

void main(){
    float dxdx = dx.x * dx.x;
    float dydy = dy.y * dy.y;
    x = ((x0(uv + dx) + x0(uv - dx)) * dydy + (x0(uv + dy) + x0(uv - dy)) * dxdx + alpha * b(uv)) * rBeta;
}