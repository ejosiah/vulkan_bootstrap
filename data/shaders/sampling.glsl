#ifndef SAMPLING_GLSL
#define SAMPLING_GLSL


vec3 uniformSampleSphere(vec2 uv){
    float z = 1 - 2 * uv.x;
    float r = sqrt(max(0, 1 - z * z));
    float phi = 2 * 3.141592653589793238462643 * uv.y;
    return vec3(r * cos(phi), r * sin(phi), z);
}

vec3 hemisphereRandom(vec2 r) {
    vec3 s = uniformSampleSphere(r);
    return vec3(s.x, s.y, abs(s.z));
}

#endif // SAMPLING_GLSL