#version 460 core


layout (location = 0) in struct {
    vec3 worldPos;
    vec3 normal;
    vec3 eyes;
} vIn;

layout(location = 0) out vec4 fragColor;

void main(){
    vec3 E = normalize(vIn.eyes - vIn.worldPos);
    vec3 L = E;
    vec3 H = normalize(E + L);
    vec3 N = normalize(vIn.normal);
    N = gl_FrontFacing ? N : -N;
    vec3 diff = vec3(1, 0, 0) * max(0, dot(N, L));
    vec3 spec = vec3(1) * max(0, pow(dot(N, H), 50));
    fragColor = vec4(diff + spec, 1);
}