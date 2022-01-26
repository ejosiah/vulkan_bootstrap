#version 460 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tanget;
layout(location = 3) in vec3 bitangent;
layout(location = 4) in vec3 color;
layout(location = 5) in vec2 uv;

layout(push_constant) uniform UniformBufferObject{
    mat4 model;
    mat4 view;
    mat4 proj;
};

layout(location = 0) out vec3 vColor;
layout(location = 1) out vec2 vUv;
layout(location = 2) out vec3 vPos;
layout(location = 3) out vec3 eyes;
layout(location = 4) out vec3 lightPos;
layout(location = 5) out vec3 vNormal;

bool isBlack(vec3 color){
    return all(equal(color, vec3(0)));
}

void main(){
    vec4 worldPos = model * vec4(position, 1.0);
    vColor = !isBlack(color) ? color : vec3(0.4);
    vPos = worldPos.xyz;
    vUv = uv;
    vNormal = mat3(model) * normal;
    eyes = (inverse(view) * vec4(0, 0, 0, 1)).xyz;
    lightPos = eyes;

    gl_PointSize = 2.0;
    gl_Position = proj * view * worldPos;
}