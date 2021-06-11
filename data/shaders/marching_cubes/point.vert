#version 460 core

layout(push_constant) uniform Constants{
    mat4 model;
    mat4 view;
    mat4 projection;
    uint config;
};

layout(location = 0) in vec3 position;
layout(location = 0) out vec4 vColor;

void main(){
    uint val = 1 << gl_VertexIndex;
    vColor = (val & config) != 0 ? vec4(0, 0, 1, 1) : vec4(1, 0, 0, 1);
    gl_PointSize = 10.0;
    gl_Position = projection * view * model * vec4(position, 1);
}