#version 450
#define PI 3.14159265358979
layout(isolines, equal_spacing, ccw) in;

layout(push_constant) uniform Constants{
    int levels[2];
    float angle;
    float radius;
    int clockwise;
};

layout(set = 0, binding = 0) uniform MVP{
    mat4 model;
    mat4 view;
    mat4 projection;
};

void main(){
    float u = gl_TessCoord.x * angle/360;
    float theta = u * 2 * PI;
    float r = radius;
    theta = bool(clockwise) ? -theta : theta;

    vec4 p = vec4(r * cos(theta), r * sin(theta), 0, 1);

    gl_Position = projection * view * model * p;
}