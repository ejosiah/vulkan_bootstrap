#version 450

layout(isolines, equal_spacing, ccw) in;

layout(set = 0, binding = 0) uniform MVP{
    mat4 model;
    mat4 view;
    mat4 projection;
};

void initHermiteMetrics(inout float H[4]){
    float u = gl_TessCoord.x;
    float uu = u * u;
    float uuu = uu * u;
    H[0] = 2 * uu - 3 * uu + 1;
    H[1] = -2 * uuu + 3 * uu;
    H[2] = uuu - 2 * uu + u;
    H[3] = uuu - uu;
}

void main(){

    vec4 p0 = gl_in[0].gl_Position;
    vec4 t0 = gl_in[1].gl_Position;
    vec4 P1 = gl_in[2].gl_Position;
    vec4 t1 = gl_in[3].gl_Position;

    float H[4];
    initHermiteMetrics(H);

    vec4 p = H[0] * p0 + H[1] * P1 + H[2] * t0 + H[3] * t1;
    gl_Position = projection * view * model * p;
}