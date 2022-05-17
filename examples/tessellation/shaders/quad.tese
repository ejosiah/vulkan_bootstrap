#version 450

layout(quads, equal_spacing, ccw) in;

void main(){
    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;

    vec4 p0 = gl_in[0].gl_Position;
    vec4 p1 = gl_in[1].gl_Position;
    vec4 p2 = gl_in[2].gl_Position;
    vec4 p3 = gl_in[3].gl_Position;

    vec4 p = mix(mix(p0, p1, u), mix(p3, p2, u), v);

    gl_Position = p;
}