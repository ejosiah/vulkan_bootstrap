#version 460

#define SQRT_3_INV 0.57735026918962576450914878050196

layout(push_constant) uniform Material{
layout(offset = 192)
    vec4 color;
} material;

layout(location = 0) in struct {
    vec3 position;
    vec3 normal;
    vec3 eyes;
} frag_in;

layout(location = 0) out vec4 fragColor;

void main(){
    vec3 N = normalize(frag_in.normal);
    vec3 eyeDir = frag_in.eyes - frag_in.position;
    vec3 E = normalize(eyeDir);
    vec3 L = E;
    vec3 H = normalize(E + L);

    float shine = 200;
    vec3 color = material.color.rgb;

    float diffuse = max(0, dot(N, L));
    float specular = max(0, pow(dot(N, H), shine));
    vec3 radiance = color * (diffuse + specular);


    float alpha = material.color.a;
    fragColor = vec4(radiance, alpha);
}