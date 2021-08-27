#ifndef SPIKY_KERNEL_GLSL
#define SPIKY_KERNEL_GLSL

#define PI 3.1415926535897932384626433832795

float spiky_kernel(float h, float distance){
    if (distance >= h) {
        return 0.0;
    } else {
        float h3 = h * h * h;
        float x = 1.0 - distance / h;
        return 15.0 / (PI * h3) * x * x * x;
    }
}

float spiky_firstDerivative(float h, float distance){
    if (distance >= h) {
        return 0.0;
    } else {
        float h4 = h * h * h * h;
        float x = 1.0 - distance / h;
        return -45.0 / (PI * h4) * x * x;
    }
}

float spiky_secondDerivative(float h, float distance){
    if (distance >= h) {
        return 0.0;
    } else {
        float h5 = h * h * h * h * h;
        float x = 1.0 - distance / h;
        return 90.0 / (PI * h5) * x;
    }
}
vec3 spiky_gradient(float h, float distance, vec3 dirToCenter){
    return -spiky_firstDerivative(h, distance) * dirToCenter;
}

vec3 spiky_gradient(float h, vec3 point){
    float dist = length(point);
    if(dist > 0.0) return spiky_gradient(h, dist, point / dist);
    return vec3(0);
}

    #endif // SPIKY_KERNEL_GLSL