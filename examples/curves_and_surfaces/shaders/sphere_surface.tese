#version 450
#define PI 3.14159265358979
layout(quads, equal_spacing, ccw) in;

layout(push_constant) uniform TESS_LEVELS{
    float outer;
    float inner;
    float maxU;
    float maxV;
    float radius;
};

layout(location = 0) out struct {
    vec3 normal;
    vec2 uv;
} v_out;


void main(){
    float u = min(gl_TessCoord.x, maxU);
    float v = min(gl_TessCoord.y, maxV);

    float theta = 2 * u * PI;
    float phi = v * PI;

    vec4 p = vec4(1);
    p.x = radius * cos(theta) * sin(phi);
    p.y = radius * cos(phi);
    p.z = radius * sin(theta) * sin(phi);

    v_out.normal = normalize(p.xyz);
    v_out.uv = vec2(u, v);
    gl_Position = p;
}