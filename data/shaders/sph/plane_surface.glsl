#ifndef PLANE_SURFACE_GLSL
#define PLANE_SURFACE_GLSL

struct Plane{
    vec3 normal;
    float d;
};

vec3 closestPoint(Plane p, vec3 q){
    float t = (dot(p.normal, q) - p.d)/dot(p.normal, p.normal);
    return q - t * p.normal;
}

Plane createPlane(vec3 normal, vec3 point){
    Plane p;
    p.normal = normal;
    p.d = dot(normal, point);
    return p;
}

#endif // PLANE_SURFACE_GLSL