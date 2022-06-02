#version 450
#extension GL_GOOGLE_include_directive : enable
#define PI 3.14159265358979

#include "octahedral.glsl"

layout(quads, equal_spacing, ccw) in;

layout(push_constant) uniform TESS_LEVELS{
    float outer;
    float inner;
    float maxU;
    float maxV;
};


const vec4 bc = vec4(1, 3, 3, 1);
const float epsilon = 0.0001;

float B(int i, float u){
    return bc[i] * pow(1 - u, 3 - i) * pow(u, i);
}

float dB(int i, float u){
    float n = 3;
    return
        bc[i] * (i * pow(u, i-1) * pow(1 - u, n - i) - pow(u, i) * (n - i) * pow(1 - u, n - i - 1));
}

vec4 f(float u, float v, int i, int j){
    int ij = i * 4 + j;
    return B(i, u) * B(j, v) * gl_in[ij].gl_Position;
}

vec4 dfdu(float u, float v, int i, int j){
    int ij = i * 4 + j;
    return dB(i, u) * B(j, v) * gl_in[ij].gl_Position;
}

vec4 dfdv(float u, float v, int i, int j){
    int ij = i * 4 + j;
    return B(i, u) * dB(j, v) * gl_in[ij].gl_Position;
}

layout(location = 0) out struct {
    vec3 normal;
    vec2 uv;
} v_out;

void main(){
    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;

    vec4 p = vec4(0);
    vec3 t = vec3(0);
    vec3 b = vec3(0);
    for(int i = 0; i < 4; i++){
        for(int j = 0; j < 4; j++){
            int ij = i * 4 + j;
            p += f(u, v, i, j);
            t += dfdu(u, v, i, j).xyz;
            b += dfdv(u, v, i, j).xyz;
        }
    }
    v_out.normal = normalize(cross(t, b));

    vec2 octUV = octEncode(normalize(p.xyz));
    v_out.uv = .5 * octUV + 5;
    gl_Position = p;

}