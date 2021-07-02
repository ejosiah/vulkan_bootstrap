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

void getNearByKeys(vec3 position, float gridSpacing, vec3 resolution, out int keys[8]){
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

void getNearByKeys2d(vec3 point, float gridSpacing, out int keys[8]){

}

void getNearByKeys3d(vec3 point, float gridSpacing, out int keys[8]){

}

#endif // POINT_HASH_GRID_GLSL