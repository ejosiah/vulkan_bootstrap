#define PI 3.1415926535897932384626433832795
#define TWO_PI 6.283185307179586476925286766559
#define EPSILON 0.000001

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

struct Cylinder{
    vec3 bottom;
    vec3 top;
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

    if(c > 0 && b > 0) return false; // ray origin is outside and facing away from sphere

    float discr = b * b - c * a;
    if(discr < 0) return false; // no real roots

    t = -b - sqrt(discr);
    t /= a;

    if(t < 0) t = 0; // ray origin is inside sphere
    return true;
}

bool cylinder_ray_test(Cylinder cylinder, Ray r, out float t){
    vec3 d = cylinder.top - cylinder.bottom;
    vec3 m = r.origin - cylinder.bottom;
    vec3 n = d;

    float md = dot(m, d);
    float nd = dot(n, d);
    float dd = dot(d, d);

    // Test if segment fully outside either endcap of cylinder
    if(md < 0 && md + nd < 0) return false; // segment outside of bottom side of cylinder
    if(md > dd && md + nd > dd) return false; // segment outside of top side of cylinder

    float nn = dot(n, n);
    float mn = dot(m, n);
    float a = dd * nn - nd * nd;
    float k = dot(m, m) - cylinder.radius * cylinder.radius;
    float c = dd * k - md * md;

    if(abs(a) < EPSILON){
        // segment runs paralle to cylinder axis
        if(c > 0) return false;
        // Now known that segment intersects cylinder; figure out how it intersects
        if(md < 0) t = -mn/nn;  // intersects segment agianst 'bottom' endcap
        else if (md > dd) t = (nd - mn) /nn; // intersect segment against 'top' endcap
        else t = 0; // 'a' lies inside cylinder;
        return true;
    }
    float b = dd * mn - nd * md;
    float discr = b * b - a * c;
    if(discr < 0) return false; // no real roots; no intersection

    t = (-b - sqrt(discr)) / a;
    if(t <  0) return false;
    if(md + t * nd < 0){
        // intersection outside cylinder on 'bottom' side
        if(nd <= 0) return false; // segment pointing away from endcap
        t = -md / nd;
        // keep intersection if dot(ray(t) - bottom, ray(t) - bottom) <= radius^2
        return k + 2 * t * (mn + t * nn) <= 0;
    }else if (md + t * nd > dd){
        // intersection outside cylinder on 'top' side
        if(nd >= 0) return false;   // segment pointing away from endcap
        t = (dd - md)/nd;
        // keep intersection if dot(ray(t) - top, ray(t) - top) <= radius^2
        return k + dd - 2 * md + t * (2 * (mn - nd) + t * nn) <= 0.0f;
    }
    return true;
}

float remap(float value, float oldMin, float oldMax, float newMin, float newMax){
    return (((value - oldMin) / (oldMax - oldMin)) * (newMax - newMin)) + newMin;
}

vec3 remap(vec3 value, vec3 oldMin, vec3 oldMax, vec3 newMin, vec3 newMax){
    return (((value - oldMin) / (oldMax - oldMin)) * (newMax - newMin)) + newMin;
}

vec2 remap(vec2 value, vec2 oldMin, vec2 oldMax, vec2 newMin, vec2 newMax){
    return (((value - oldMin) / (oldMax - oldMin)) * (newMax - newMin)) + newMin;
}

vec2 getUV(Sphere s, Ray r, float t){
    vec3 x = r.origin + r.direction * t;
    vec3 p = x - s.center;
    float u = acos(p.y/s.radius);
    float v = atan(p.x, p.z);
    vec2 uv;
    uv.x = u / (TWO_PI);
    uv.y = remap(v, -PI, PI, 0, 1);;

    return uv;
}

void getTangents(Sphere s, Ray r, float t, out vec3 tangent, out vec3 bitangent);

//vec2 getUV(Plane p, Ray r, float t);
//
//vec2 getUV(Cylinder cylinder, Ray r, float t);