#version 450

layout(location = 0) in struct {
    vec3 normal;
    vec3 worldPos;
    vec3 lightPos;
    vec3 eyePos;
} v_in;

layout(location = 0) out vec4 fragColor;

void main(){
    vec3 albedo = vec3(1, 0, 0);
    vec3 N = normalize(v_in.normal);
    N = gl_FrontFacing ? N : -N;
    vec3 L = normalize(v_in.lightPos - v_in.worldPos);
    vec3 E = normalize(v_in.eyePos - v_in.worldPos);
    vec3 H = normalize(E + L);

    vec3 color = albedo * max(0, dot(N, L));
    color += vec3(1) * pow(max(0, dot(N, H)), 250);

    fragColor = vec4(color, 1);
}