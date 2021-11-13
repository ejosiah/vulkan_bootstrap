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

/**
Generate the ith 2D Hammersley point out of N on the unit square [0, 1]^2
\cite http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html */
vec2 hammersleySquare(uint i, const uint N) {
    vec2 P;
    P.x = float(i) * (1.0 / float(N));

    i = (i << 16u) | (i >> 16u);
    i = ((i & 0x55555555u) << 1u) | ((i & 0xAAAAAAAAu) >> 1u);
    i = ((i & 0x33333333u) << 2u) | ((i & 0xCCCCCCCCu) >> 2u);
    i = ((i & 0x0F0F0F0Fu) << 4u) | ((i & 0xF0F0F0F0u) >> 4u);
    i = ((i & 0x00FF00FFu) << 8u) | ((i & 0xFF00FF00u) >> 8u);
    P.y = float(i) * 2.3283064365386963e-10; // / 0x100000000

    return P;
}

#endif // SAMPLING_GLSL