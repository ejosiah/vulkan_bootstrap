#ifndef POINT_HASH_GRID_GLSL
#define POINT_HASH_GRID_GLSL

vec3 wrap(vec3 bucketIndex, vec3 resolution){
    bucketIndex = mod(bucketIndex, resolution);
    return bucketIndex + (1 - step(0, bucketIndex)) * resolution;
}

int toHashKey(vec3 bucketIndex, vec3 resolution){
    return int((bucketIndex.z * resolution.y + bucketIndex.y) * resolution.x + bucketIndex.x);
}

int getHashKey(vec3 point, vec3 resolution, float gridSpacing){
    vec3 bucketIndex = floor(point/gridSpacing);
    bucketIndex = wrap(bucketIndex, resolution);
    return toHashKey(bucketIndex, resolution);
}

void getNearByKey(vec3 point, float gridSpacing, out int keys[8]){

}

#endif // POINT_HASH_GRID_GLSL