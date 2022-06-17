#version 450
#define PI 3.14159265358979
layout(isolines, equal_spacing, ccw) in;

layout(set = 0, binding = 0) uniform MVP{
    mat4 model;
    mat4 view;
    mat4 projection;
};

layout(push_constant) uniform constants {
    vec2 center;
    float brushRadius;
};

void main(){
    float theta = gl_TessCoord.x * 2 * PI;
    float r = brushRadius;
    vec2 p = center + vec2(r * cos(theta), r * sin(theta));
    gl_Position = projection * view * model * vec4(p, 0, 1);
}