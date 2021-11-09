#version 460 core

const float shine = 120;

layout(location = 0) in struct {
    vec3 pos;
    vec3  normal;
    vec3 color;
    vec3 lightDir;
    vec3 eyes;
} v_in;

layout(location = 0) out vec4 fragColor;

void main(){
    vec3 L = normalize(v_in.eyes);
    vec3 N = normalize(v_in.normal);
    vec3 E = normalize(v_in.eyes - v_in.pos);
    vec3 H = normalize(E + L);

    vec3 color = max(0, dot(L, N)) * v_in.color;
    color += max(0, pow(dot(H, N), shine)) * v_in.color;
    fragColor = vec4(color, 1);
}