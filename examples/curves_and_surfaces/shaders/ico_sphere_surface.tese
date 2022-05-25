#version 450
#extension GL_GOOGLE_include_directive : enable
#define PI 3.14159265358979

#include "octahedral.glsl"

layout(triangles, equal_spacing, ccw) in;

layout(push_constant) uniform TESS_LEVELS{
    float outer;
    float inner;
    float maxU;
    float maxV;
    float maxW;
    float radius;
};

layout(location = 0) out struct {
    vec3 normal;
    vec2 uv;
} v_out;


void main(){
    float u = min(gl_TessCoord.x, maxU);
    float v = min(gl_TessCoord.y, maxV);
    float w = min(gl_TessCoord.z, maxW);

    vec3 p0 = gl_in[0].gl_Position.xyz * u;
    vec3 p1 = gl_in[1].gl_Position.xyz * v;
    vec3 p2 = gl_in[2].gl_Position.xyz * w;

    vec3 p = normalize(p0 + p1 + p2);
    v_out.normal = -p;
    p *= radius;

//    float theta = atan(p.z, p.x);
//    float phi = acos(p.y/radius);
//
//    v_out.uv.x = theta/(2 * PI) + .5;
//    v_out.uv.y = phi/PI;

    vec2 octUV = octEncode(normalize(p.xyz));
    v_out.uv = .5 * octUV + 5;

    gl_Position = vec4(p, 1);

}