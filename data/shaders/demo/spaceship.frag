#version 460 core

const vec3 globalAmbient = vec3(0);
const vec3 Light = vec3(1);

layout(constant_id = 0) const uint materialOffset = 192;

layout(push_constant) uniform MATERIAL {
    layout(offset = 192)
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

float saturate(float x){
    return max(0, x);
}

void main(){
    vec3 N = normalize(vNormal);
    vec3 L = normalize(lightPos - vPos);
    vec3 E = normalize(eyes - vPos);
    vec3 H = normalize(E + L);
    vec3 R = reflect(-L, N);

    vec3 color = Light * (saturate(dot(L, N)) * diffuse + saturate(pow(dot(H, N), shininess)) * specular);
    fragColor = vec4(color, 1);
}