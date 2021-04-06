#version 460 core

const vec3 globalAmbient = vec3(0.2);
const vec3 Light = vec3(1);

layout(push_constant) uniform MATERIAL {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

layout(location = 0) in vec3 vPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec3 eyes;
layout(location = 3) in vec3 lightPos;

layout(location = 0) out vec4 fragColor;

vec3 saturate(float x){
    return clamp(0, 1, x);
}

void main(){
    vec3 N = normalize(vNormal);
    vec3 L = normalize(lightPos - vPos);
    vec3 E = normalize(eyes - vPos);
    vec3 H = normalize(E + L);

    vec3 color = globalAmbient * ambient + Light * (saturate(dot(L, N)) + saturate(pow(dot(H, N), shininess)));
    fragColor = vec4(color, 1);
}