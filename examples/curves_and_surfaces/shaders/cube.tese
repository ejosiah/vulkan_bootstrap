#version 450
#extension GL_GOOGLE_include_directive : enable
#define PI 3.14159265358979

#include "octahedral.glsl"

layout(quads, equal_spacing, ccw) in;

layout(push_constant) uniform TESS_LEVELS{
    vec3 scale;
    float outer;
    float inner;
    float max_u;
    float max_v;
    int shouldNormalize;
};

layout(location = 0) out struct {
    vec3 normal;
    vec2 uv;
} v_out;

void main(){
    float u = min(gl_TessCoord.x, max_u);
    float v = min(gl_TessCoord.y, max_v);

    vec3 p0 = gl_in[0].gl_Position.xyz;
    vec3 p1 = gl_in[1].gl_Position.xyz;
    vec3 p2 = gl_in[2].gl_Position.xyz;
    vec3 p3 = gl_in[3].gl_Position.xyz;

    vec3 p = mix(mix(p0, p1, u), mix(p3, p2, u), v);
    vec3 n;

    if(bool(shouldNormalize)){
        n = normalize(p).xyz;
        float r = max(max(scale.x, scale.y), scale.z);
        p = n * r;
        float theta = atan(p.z, p.x);
        float phi = acos(p.y/r);

//        v_out.uv.x = theta/(2 * PI) + .5;
//        v_out.uv.y = phi/PI;
        vec2 octUV = octEncode(normalize(p.xyz));
        v_out.uv = .5 * octUV + 5;
    }else{
        n = cross(p1.xyz - p0.xyz, p2.xyz - p0.xyz);
        p *= scale;
        v_out.uv = vec2(u, v);
    }

    v_out.normal = n;

    gl_Position = vec4(p, 1);

}