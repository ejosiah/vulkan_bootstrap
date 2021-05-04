#version 460 core
#extension GL_EXT_scalar_block_layout : enable

const vec3 globalAmbient = vec3(0);
const vec3 Light = vec3(1);

layout(constant_id = 0) const uint materialOffset = 192;

layout(set = 0, binding = 0) uniform MATERIAL {
    vec3 diffuse;
    vec3 ambient;
    vec3 specular;
    vec3 emission;
    vec3 transmittance;
    float shininess;
    float ior;
    float opacity;
    float illum;
};

layout(set = 0, binding = 1) uniform sampler2D ambientMap;
layout(set = 0, binding = 2) uniform sampler2D diffuseMap;
layout(set = 0, binding = 3) uniform sampler2D specularMap;
layout(set = 0, binding = 4) uniform sampler2D normalMap;
layout(set = 0, binding = 5) uniform sampler2D ambientOcclusionMap;

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
    fragColor = vec4(N, 1);
}