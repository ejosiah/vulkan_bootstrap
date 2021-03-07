#version 460 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 color;
layout(location = 3) in vec2 uv;

layout(set = 0, binding = 0) uniform UniformBufferObject{
    mat4 model;
    mat4 view;
    mat4 proj;
};

layout(location = 0) out vec3 vColor;
layout(location = 1) out vec2 vUv;
vec3 colors[3] = vec3[](
    vec3(1, 0, 0.0),
    vec3(0, 1, 0),
    vec3(0, 0, 1)
);

void main(){
    gl_Position = proj * view * model * vec4(position, 1.0);
//    gl_Position = vec4(position, 1.0);
//    vColor = vec3(uv, 0.0);
    vColor = vec3(1);
    vUv = uv;
  //  vUv.y *= -1;
}