#version 460

layout(location = 0) in struct {
    vec3 position;
    vec3 normal;
    vec3 color;
} v_in;

layout(location = 0) out vec4 fragColor;

const vec3 lightDir = vec3(1);
const vec3 globalAmbience = vec3(0.3);
void main(){
    vec3 N = normalize(v_in.normal);
    vec3 L = normalize(lightDir);

    vec3 albedo = v_in.color;
    vec3 color = globalAmbience * albedo + max(0, dot(N, L)) * albedo;
    fragColor = vec4(color, 1);
}