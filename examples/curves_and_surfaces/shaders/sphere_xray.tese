#version 450
#define PI 3.14159265358979

layout(isolines, equal_spacing, ccw) in;

layout(set = 0, binding = 0) uniform MVP{
    mat4 model;
    mat4 view;
    mat4 projection;
};

layout(push_constant) uniform TESS_LEVELS{
layout(offset = 8)
    float outer;
    float inner;
    float max_u;
    float max_v;
    float radius;
};

void main(){
    float u = min(gl_TessCoord.x, max_u);
    float v = min(gl_TessCoord.y, max_v);

    float theta = 2 * u * PI;
    float phi = u * PI;

    vec4 p = vec4(1);
    p.x = radius * cos(theta) * sin(phi);
    p.y = radius * cos(phi);
    p.z = radius * sin(theta) * sin(phi);

    gl_Position = projection * view * model * p;
}