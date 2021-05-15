#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

#define PI 3.1415926535897932384626433832795
#define TWO_PI 6.283185307179586476925286766559

#include "implicits.glsl"
#include "common.glsl"

layout(set = 1, binding = 0) buffer SPHERES{
    Sphere spheres[];
};

layout(set = 1, binding = 1) buffer PLANES{
    Plane planes[];
};

hitAttributeEXT vec2 attribs;

void main(){
    Ray ray = Ray(gl_WorldRayOriginEXT, gl_WorldRayDirectionEXT);
    float t = 0;

    if(gl_InstanceCustomIndexEXT == IMPLICIT_TYPE_SPHERE){

        Sphere sphere = spheres[gl_PrimitiveID];
        if (sphere_ray_test(sphere, ray, t)){
            vec3 p = ray.origin + ray.direction * t;
            float u = atan(p.x, p.z);
            float v = acos(p.y/sphere.radius);
            attribs.x = u * 5 / (TWO_PI);
            attribs.y = v * 5 / (PI);

            reportIntersectionEXT(t, IMPLICIT_TYPE_SPHERE);
        }
    }else if (gl_InstanceCustomIndexEXT == IMPLICIT_TYPE_PLANE){
        Plane plane = planes[gl_PrimitiveID];
        if(plane_ray_test(plane, ray, t)){
            vec3 x, y;
            vec3 p = ray.origin + ray.direction * t;
            orthonormalBasis(plane.normal, x, y);
            attribs.x = dot(p, x);
            attribs.y = dot(p, y);
            reportIntersectionEXT(t, IMPLICIT_TYPE_PLANE);
        }
    }

}