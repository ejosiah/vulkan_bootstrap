#ifndef SAMPLING_GLSL
#define SAMPLING_GLSL


vec3 uniformSampleSphere(vec2 uv){
    float z = 1 - 2 * uv.x;
    float r = sqrt(max(0, 1 - z * z));
    float phi = 2 * 3.141592653589793238462643 * uv.y;
    return vec3(r * cos(phi), r * sin(phi), z);
}

#endif // SAMPLING_GLSL