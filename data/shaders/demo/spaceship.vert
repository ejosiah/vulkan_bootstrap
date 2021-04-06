#version 460 core

layout(push_constant) uniform MVP {
    mat4 model;
    mat4 view;
    mat4 projection;
};

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal;

layout(location = 0) out vec3 vPos;
layout(location = 1) out vec3 vNormal;
layout(location = 2) out vec3 eyes;
layout(location = 3) out vec3 lightPos;

void main(){
    vec4 worldPos = model * position;
    vPos = worldPos.xyz;
    vNormal = mat3(model) * normal;
    eyes = (inverse(view) * vec4(0, 0, 0, 1)).xyz;
    lightPos = eyes;

    gl_Position = projection * view * worldPos;
}