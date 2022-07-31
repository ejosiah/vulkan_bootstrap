#version 450 core

layout(set = 0, binding = 0) uniform sampler2D solution;
layout(set = 1, binding = 0) uniform sampler2D unknown;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 x;

layout(push_constant) uniform Constants {
    float alpha;
    float rBeta;
    int isVectorField;
};

vec4 applyBoundaryCondition(vec2 uv, vec4 u){
    if(uv.x <= 0 || u.x >= 1){
        u.x *= -1;
    }
    if(u.y <= 0 || u.y >= 1){
        u.y *= -1;
    }
    return u;
}

vec4 b(vec2 coord){
    vec4 _b = texture(solution, coord);
    if(isVectorField == 1){
        return applyBoundaryCondition(coord, _b);
    }
    return _b;
}

vec4 x0(vec2 coord){
    vec4 _x0 = texture(unknown, coord);
    if(isVectorField == 1){
        return applyBoundaryCondition(coord, _x0);
    }
    return _x0;
}

void main(){
    vec2 size = textureSize(unknown, 0);
    vec3 delta = vec3(1.0/size, 0);
    vec2 dx = delta.xz;
    vec2 dy = delta.zy;

    x = (x0(uv + dx) + x0(uv - dx) + x0(uv + dy) + x0(uv - dy) + alpha * b(uv)) * rBeta;
}