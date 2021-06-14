#ifndef SDF_MODEl_GLSL
#define SDF_MODEl_GLSL

const float EPSILON = 0.000001;

float intersectSDF(float distA, float distB){
    return max(distA, distB);
}

float unionSDF(float distA, float distB){
    return min(distA, distB);
}

float differenceSDF(float distA, float distB){
    return max(distA, -distB);
}

float cubeSDF(vec3 p, vec3 size){
    vec3 q = abs(p) - size;
    return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0);
}

float sphereSDF(vec3 p, float r){
    return length(p) - r;
}

float sceneSDF(vec3 p){
    float d0 = sphereSDF(p/1.2, 1.0) * 1.2;
    float d1 = min(min(sphereSDF(vec3(p.xy, 0), 0.5), sphereSDF(vec3(p.xz, 0), 0.5)), sphereSDF(vec3(p.yz, 0), 0.5));
    float d2 = cubeSDF(p, vec3(.8)) - 0.05;


    float d =  max(max(d0, d2), -d1);

    float d3 = sphereSDF(abs(p) - vec3(.8, 0, 0), 0.3);
    d = min(d, d3);

    float d4 = sphereSDF(abs(p) - vec3(0, 0.8, 0), 0.3);
    d = min(d, d4);

    float d5 = sphereSDF(abs(p) - vec3(0, 0, 0.8), 0.3);
    d = min(d, d5);

    return d;
}

vec3 estimateNormal(vec3 p) {
    return normalize(vec3(
    sceneSDF(vec3(p.x + EPSILON, p.y, p.z)) - sceneSDF(vec3(p.x - EPSILON, p.y, p.z)),
    sceneSDF(vec3(p.x, p.y + EPSILON, p.z)) - sceneSDF(vec3(p.x, p.y - EPSILON, p.z)),
    sceneSDF(vec3(p.x, p.y, p.z  + EPSILON)) - sceneSDF(vec3(p.x, p.y, p.z - EPSILON))
    ));
}

#endif // SDF_MODEl_GLSL