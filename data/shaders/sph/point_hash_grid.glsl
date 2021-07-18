#ifndef POINT_HASH_GRID_GLSL
#define POINT_HASH_GRID_GLSL

#include "../glsl_cpp_bridge.glsl"

vec3 wrap(vec3 bucketIndex, vec3 resolution){
    return mod(bucketIndex, resolution);
}

int toHashKey(vec3 bucketIndex, vec3 resolution){
    return int((bucketIndex.z * resolution.y + bucketIndex.y) * resolution.x + bucketIndex.x);
}

int getHashKey(vec3 point, vec3 resolution, float gridSpacing){
    vec3 bucketIndex = floor(point/gridSpacing);
    bucketIndex = wrap(bucketIndex, resolution);
    return toHashKey(bucketIndex, resolution);
}

bool is2dGrid(vec3 resolution){
    return (resolution.z - 1.0) <= 0.0;
}

void getNearByKeys2d(vec3 position, float gridSpacing, vec3 resolution, inout int keys[8]){
    vec3 originIndex = floor(position/gridSpacing);
    vec3 nearByBucketIndices[8];
    for(int i = 0; i < 4; i++){
        nearByBucketIndices[i] = originIndex;
    }

    if ((originIndex.x + 0.5) * gridSpacing <= position.x) {
        nearByBucketIndices[2].x += 1;
        nearByBucketIndices[3].x += 1;
    } else {
        nearByBucketIndices[2].x -= 1;
        nearByBucketIndices[3].x -= 1;
    }

    if ((originIndex.y + 0.5) * gridSpacing <= position.y) {
        nearByBucketIndices[1].y += 1;
        nearByBucketIndices[3].y += 1;
    } else {
        nearByBucketIndices[1].y -= 1;
        nearByBucketIndices[3].y -= 1;
    }

    for (int i = 0; i < 4; i++) {
        vec3 wrappedIndex = wrap(nearByBucketIndices[i], resolution);
        keys[i] = toHashKey(wrappedIndex, resolution);
    }
}

void getNearByKeys3d(vec3 position, float gridSpacing, vec3 resolution, inout int keys[8]){
    vec3 originIndex = floor(position/gridSpacing);
    vec3 nearByBucketIndices[8];
    for(int i = 0; i < 8; i++){
        nearByBucketIndices[i] = originIndex;
    }

    if ((originIndex.x + 0.5f) * gridSpacing <= position.x) {
        nearByBucketIndices[4].x += 1;
        nearByBucketIndices[5].x += 1;
        nearByBucketIndices[6].x += 1;
        nearByBucketIndices[7].x += 1;
    } else {
        nearByBucketIndices[4].x -= 1;
        nearByBucketIndices[5].x -= 1;
        nearByBucketIndices[6].x -= 1;
        nearByBucketIndices[7].x -= 1;
    }

    if ((originIndex.y + 0.5f) * gridSpacing <= position.y) {
        nearByBucketIndices[2].y += 1;
        nearByBucketIndices[3].y += 1;
        nearByBucketIndices[6].y += 1;
        nearByBucketIndices[7].y += 1;
    } else {
        nearByBucketIndices[2].y -= 1;
        nearByBucketIndices[3].y -= 1;
        nearByBucketIndices[6].y -= 1;
        nearByBucketIndices[7].y -= 1;
    }

    if ((originIndex.z + 0.5f) * gridSpacing <= position.z) {
        nearByBucketIndices[1].z += 1;
        nearByBucketIndices[3].z += 1;
        nearByBucketIndices[5].z += 1;
        nearByBucketIndices[7].z += 1;
    } else {
        nearByBucketIndices[1].z -= 1;
        nearByBucketIndices[3].z -= 1;
        nearByBucketIndices[5].z -= 1;
        nearByBucketIndices[7].z -= 1;
    }

    for (int i = 0; i < 8; i++) {
        vec3 wrappedIndex = wrap(nearByBucketIndices[i], resolution);
        keys[i] = toHashKey(wrappedIndex, resolution);
    }
}


void getNearByKeys(vec3 position, float gridSpacing, vec3 resolution, out int keys[8]){
    for(int i = 0; i < 8; i++){
        keys[i] = -1;
    }
    if(is2dGrid(resolution)){
        getNearByKeys2d(position, gridSpacing, resolution, keys);
    }else {
        getNearByKeys3d(position, gridSpacing, resolution, keys);
    }

}

#endif // POINT_HASH_GRID_GLSL