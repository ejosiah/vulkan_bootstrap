#ifndef BOUNDING_BOX_GLSL
#define BOUNDING_BOX_GLSL

struct BoundingBox{
    vec3 min;
    vec3 max;
};

vec3 remap(vec3 x, BoundingBox oldBox, BoundingBox newBox){
    return mix(newBox.min, newBox.max, (x - oldBox.min)/(oldBox.max - oldBox.min));
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
    return (box.min + box.max) * vec3(0.5);
}

vec3 closedPoint(BoundingBox box, vec3 p){
   return clamp(p, box.min, box.max);
}

bool contains(BoundingBox box, vec3 p){
    if (box.max.x < p.x || box.min.x > p.x) {
        return false;
    }

    if (box.max.y < p.y || box.min.y > p.y) {
        return false;
    }

    if (box.max.z < p.z || box.min.z > p.z) {
        return false;
    }

    return true;
}

#endif // BOUNDING_BOX_GLSL