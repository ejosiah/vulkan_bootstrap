#define st(p) (bool(ensureBoundaryCondition) ? p : fract(p))

bool checkBoundary(vec3 uv){
    return bool(ensureBoundaryCondition) && (uv.x <= 0 || uv.x >= 1 || uv.y <= 0 || uv.y >= 1 || uv.z <= 0 || uv.z >= 1);
}

vec3 applyBoundaryCondition(vec3 uv, vec3 u){
    if(checkBoundary(uv)){
        u *= -1;
    }
    return u;
}

vec4 applyBoundaryCondition(vec3 uv, vec4 u){
    if(checkBoundary(uv)){
        u *= -1;
    }
    return u;
}