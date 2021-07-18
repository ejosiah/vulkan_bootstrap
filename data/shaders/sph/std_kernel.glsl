#ifndef STD_KERNEL_GLSL
#define STD_KERNEL_GLSL

#define PI 3.14159265358979323

float kernel(float h, float distance){
    float h2 = h * h;
    if (distance * distance >= h2) return 0.0;

    float h3 = h2 * h;
    float distanceSquared = distance * distance;
    float x = 1.0 - distance * distance / h2;
    return 315.0 / (64.0 * PI * h3) * x * x * x;
}

float firstDerivative(float h, float distance){
    if (distance >= h) return 0.0;

    float h2 = h * h;
    float h5 = h2 * h2 * h;
    float x = 1.0 - distance * distance / h2;
    return -945.0f / (32.0f * PI * h5) * distance * x * x;
}

float secondDerivative(float h, float distance){
    float h2 = h * h;
    if (distance * distance >= h2) return 0.0f;

    float h5 = h2 * h2 * h;
    float x = distance * distance / h2;
    return 945.0f / (32.0f * PI * h5) * (1 - x) * (5 * x - 1);
}
vec3 gradient(float h, float distance, vec3 dirToCenter){
    return -firstDerivative(h, distance) * dirToCenter;
}

vec3 gradient(float h, vec3 point){
    float dist = length(point);
    if(dist > 0.0) return gradient(h, dist, point / dist);
    return vec3(0);
}
    
#endif // STD_KERNEL_GLSL