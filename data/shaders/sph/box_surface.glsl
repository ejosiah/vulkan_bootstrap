#ifndef BOX_SURFACE_GLSL
#define BOX_SURFACE_GLSL


#include "../glsl_cpp_bridge.glsl"
#include "../bounding_box.glsl"
#include "plane_surface.glsl"

struct BoxSurface{
    BoundingBox bounds;
    int normalFlipped;
};

vec3 closestPoint(BoxSurface box, vec3 point, OUT(vec3) o_normal, OUT(float) o_dist){
    BoundingBox bounds = box.bounds;
    Plane faces[6];
    faces[0] = createPlane(vec3(1, 0, 0), bounds.max);
    faces[1] = createPlane(vec3(0, 1, 0), bounds.max);
    faces[2] = createPlane(vec3(0, 0, 1), bounds.max);
    faces[3] = createPlane(vec3(-1, 0, 0), bounds.min);
    faces[4] = createPlane(vec3(0, -1, 0), bounds.min);
    faces[5] = createPlane(vec3(0, 0, -1), bounds.min);

    if(contains(bounds, point)){
        vec3 res = vec3(0);
        o_dist = 1e10;
        for(int i = 0; i < 6; i++){
            Plane face = faces[i];
            vec3 pointOnSurface = closestPoint(face, point);
            float d = distance(point, pointOnSurface);
            if(d < o_dist){
                res = pointOnSurface;
                o_dist = d;
                o_normal = face.normal;
            }
        }
        return res;
    }

    o_dist = 1e-10;
    vec3 res = vec3(0);

    for(int i = 0; i < 6; i++){
        Plane face = faces[i];
        vec3 pointOnSurface = closestPoint(face, point);
        vec3 dir = point - pointOnSurface;
        float cosine = dot(dir, face.normal);
        if(cosine > o_dist){
            res = pointOnSurface;
            o_dist = cosine;
            o_normal = face.normal;
        }
    }

    return res;
}

bool isPenetrating(BoxSurface box, vec3 position, float radius, OUT(vec3) normal, OUT(vec3) surfacePoint){
    float dist;
    surfacePoint = closestPoint(box, position, normal, dist);
    normal *= bool(box.normalFlipped) ? -1 : 1;
    return dot(position - surfacePoint, normal) < 0 || dist < radius;
}

#endif // BOX_SURFACE_GLSL