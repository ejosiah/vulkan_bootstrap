#version 460 core

const int repeat = 100;
const float shine = 120;

layout(location = 0) in struct {
    vec3 pos;
    vec3  normal;
    vec3 lightDir;
    vec3 eyes;
    vec2 uv;
} v_in;

layout(location = 0) out vec4 fragColor;

void main(){
    vec3 L = normalize(v_in.eyes);
    vec3 N = normalize(v_in.normal);
    vec3 E = normalize(v_in.eyes - v_in.pos);
    vec3 H = normalize(E + L);

    ivec2 id = ivec2(floor(v_in.uv * repeat));
    vec3 checker = (id.x + id.y) % 2 == 0 ? vec3(1) : vec3(0);
    vec3 color = max(0, dot(L, N)) * checker;
    color += max(0, pow(dot(H, N), shine)) * checker;
    fragColor = vec4(color, 1);
}