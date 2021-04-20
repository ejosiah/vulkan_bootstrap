#version 460 core

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec4 color;
layout(location = 3) in vec2 uv;
layout(location = 4) in vec3 tangent;
layout(location = 5) in vec3 bitangent;

layout (constant_id = 0) const float pointSize = 2.0;

layout(push_constant) uniform CONTANTS {
    mat4 model;
    mat4 view;
    mat4 projection;
    vec4 pColor;
};

layout(location = 0) out vec4 vColor;

void main(){
    vColor = pColor;
    gl_PointSize = pointSize;
    gl_Position = projection * view * model * position;
}