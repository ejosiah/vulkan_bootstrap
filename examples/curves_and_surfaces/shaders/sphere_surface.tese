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

layout(location = 0) out vec3 normal;

void main(){
    float u = min(gl_TessCoord.x, maxU);
    float v = min(gl_TessCoord.y, maxV);

    float theta = 2 * u * PI;
    float phi = v * PI;

    vec4 p = vec4(1);
    p.x = radius * cos(theta) * sin(phi);
    p.y = radius * cos(phi);
    p.z = radius * sin(theta) * sin(phi);

    normal = normalize(p.xyz);

    gl_Position = p;

}