#version 450

layout(isolines, equal_spacing, ccw) in;

layout(set = 0, binding = 0) uniform MVP{
    mat4 model;
    mat4 view;
    mat4 projection;
};

void main(){
    float u = gl_TessCoord.x;
    float u2 = u * u;
    float u3 = u2 * u;
    float _1_u = 1 - u;
    float _1_u2 = _1_u * _1_u;
    float _1_u3 = _1_u2 * _1_u;

    vec4 p0 = gl_in[0].gl_Position;
    vec4 p1 = gl_in[1].gl_Position;
    vec4 p2 = gl_in[2].gl_Position;
    vec4 p3 = gl_in[3].gl_Position;

    vec4 p = _1_u3 * p0 + 3 * _1_u2 * u * p1 + 3 * _1_u * u2 * p2 + u3 * p3;

    gl_Position = projection * view * model * p;
}