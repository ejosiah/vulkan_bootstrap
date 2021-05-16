#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

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
            attribs = getUV(sphere, ray, t);

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