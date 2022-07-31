#define st(p) (bool(ensureBoundaryCondition) ? p : fract(p))

bool checkBoundary(vec2 uv){
    return bool(ensureBoundaryCondition) && (uv.x <= 0 || uv.x >= 1 || uv.y <= 0 || uv.y >= 1);
}

vec2 applyBoundaryCondition(vec2 uv, vec2 u){
    if(checkBoundary(uv)){
        u *= -1;
    }
    return u;
}

vec4 applyBoundaryCondition(vec2 uv, vec4 u){
    if(checkBoundary(uv)){
        u *= -1;
    }
    return u;
}