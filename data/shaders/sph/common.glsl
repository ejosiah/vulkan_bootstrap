#ifndef COMMON_GLSL
#define COMMON_GLSL

struct Particle{
    vec4 position;
    vec4 color;
    vec3 velocity;
    float invMass;
};

struct BoundingBox{
    vec3 min;
    vec3 max;
};

vec3 remap(vec3 x, BoundingBox old, BoundingBox new){
    return mix(new.min, new.max, (x - old.min)/(old.max - old.min));
}

vec3 diagonal(BoundingBox box){
    return box.max - box.min;
}

float width(BoundingBox box){
    return diagonal(box).x;
}

float height(BoundingBox box){
    return diagonal(box).y;
}
float depth(BoundingBox box){
    return diagonal(box).z;
}

vec3 midPoint(BoundingBox box){
    return (box.min + box.max) * 0.5;
}


#endif // COMMON_GLSL