const uint IMPLICIT_TYPE_PLANE = 1;
const uint IMPLICIT_TYPE_SPHERE = 2;
const uint IMPLICIT_TYPE_CYLINDER = 3;
const uint IMPLICIT_TYPE_BOX = 4;

struct Plane{
    vec3 normal;
    float d;
};

struct Sphere{
    vec3 center;
    float radius;
};

struct Ray{
    vec3 origin;
    vec3 direction;
};

bool plane_ray_test(Plane p, Ray r, out float t){

    t = p.d - dot(p.normal, r.origin);
    t /= dot(p.normal, r.direction);

    return t > 0;
}

bool sphere_ray_test(Sphere s, Ray r, out float t){
    vec3 m = r.origin - s.center;
    float a = dot(r.direction, r.direction);
    float b = dot(m, r.direction);
    float c = dot(m, m) - s.radius * s.radius;

    if(c > 0 && b > 0) return false;

    float discr = b * b - c * a;
    if(discr < 0) return false;

    t = -b - sqrt(discr);
    t /= a;

    if(t < 0) t = 0;
    return true;
}