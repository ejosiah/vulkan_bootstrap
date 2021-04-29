#version 460 core

const vec3 globalAmbient = vec3(0.3);
const vec3 lightColor = vec3(1);

layout(set = 0, binding = 0) uniform sampler2D diffuseMap;

layout(push_constant) uniform LIGHT_PARAMS{
    layout(offset = 192)
    int useTexture;
    float shine;
};

layout(location = 0) in vec3 vColor;
layout(location = 1) in vec2 vUv;
layout(location = 2) in vec3 vPos;
layout(location = 3) in vec3 eyes;
layout(location = 4) in vec3 lightPos;
layout(location = 5) in vec3 vNormal;

layout(location = 0) out vec4 fragColor;

void main(){
    vec3 N = normalize(vNormal);
    N = gl_FrontFacing ? N : -N;
    vec3 L = normalize(lightPos - vPos);
    vec3 E = normalize(eyes - vPos);
    vec3 H = normalize(E + L);

   vec3 diffuse = (useTexture == 1) ? texture(diffuseMap, vUv).rgb : vColor;

   vec3 color = lightColor * max(dot(L, N), 0) * diffuse;
    color += lightColor * pow(max(dot(H, N), 0), shine);
    fragColor = vec4(color, 1);
}
