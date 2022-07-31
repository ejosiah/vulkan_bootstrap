#version 450 core

layout(set = 0, binding = 0) uniform Globals{
    vec2 dx;
    vec2 dy;
    float dt;
    int ensureBoundaryCondition;
};

layout(set = 1, binding = 0) uniform sampler2D solution;
layout(set = 2, binding = 0) uniform sampler2D unknown;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 x;

layout(push_constant) uniform Constants {
    float alpha;
    float rBeta;
    int isVectorField;
};

bool checkBoundary(vec2 uv){
    return bool(ensureBoundaryCondition) && (uv.x <= 0 || uv.x >= 1 || uv.y <= 0 || uv.y >= 1);
}

vec4 applyBoundaryCondition(vec2 uv, vec4 u){
    if(checkBoundary(uv)){
        u *= -1;
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
    x = (x0(uv + dx) + x0(uv - dx) + x0(uv + dy) + x0(uv - dy) + alpha * b(uv)) * rBeta;
}