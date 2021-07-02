#ifndef STD_KERNEL_GLSL
#define STD_KERNEL_GLSL

#define kPiD 3.14159265358979323

float kernel(float h, float distance){
    float h2 = h * h;
    float distanceSquared = distance * distance;

    if (distanceSquared >= h2) {
        return 0.0;
    } else {
        float x = 1.0 - distanceSquared / h2;
        return 4.0 / (kPiD * h2) * x * x * x;
    }
}
    
float firstDerivative(float h, float distance){
    float h2 = h * h;
    if (distance >= h) {
        return 0.0;
    } else {
        float x = 1.0 - distance * distance / h2;
        return -24.0 * distance / (kPiD * h4) * x * x;
    } 
}

float secondDerivative(float h, float distance){
    float distanceSquared = distance * distance;
    float h2 = h * h;
    if (distanceSquared >= h2) {
        return 0.0;
    } else {
        float x = distanceSquared / h2;
        return 24.0 / (kPiD * h4) * (1 - x) * (5 * x - 1);
    }
}

vec2 gradient(float h, vec2 point){
    float dist = length(point);
    if(dist > 0){
        return gradient(h, dist, point / dist);
    }else {
        return vec2(0);
    }
} 
    
vec2 gradient(float h, float distance, vec2 dirToCenter){
    return -firstDerivative(h, distance) * dirToCenter;
}
    
#endif // STD_KERNEL_GLSL