#version 450
#define HALF_PI 1.5707963267948966192313216916398
layout(isolines, equal_spacing, ccw) in;

layout(set = 0, binding = 0) uniform MVP {
    mat4 model;
    mat4 view;
    mat4 projection;
};

layout(location = 0) in float angles[];

void main(){
    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;

    vec4 p0 = gl_in[0].gl_Position;
    vec4 p1 = gl_in[1].gl_Position;
    vec4 p2 = gl_in[2].gl_Position;
    vec4 p3 = gl_in[3].gl_Position;

    vec4 p = mix(mix(p0, p1, u), mix(p3, p2, u), v);

    float angle = angles[0];
    mat2 rotor = mat2(cos(angle), sin(angle), -sin(angle), cos(angle));
    p.xy -= 0.5;
    p.xy = rotor * p.xy;
    p.xy += 0.5;

    gl_Position = projection * view * model * p;
}