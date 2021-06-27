#version 460 core

layout(location = 0) in vec4 position;
layout(location = 1) in vec4 color;

layout (constant_id = 0) const float pointSize = 5.0;

layout(set = 0, binding = 0) uniform MVP{
    mat4 model;
    mat4 view;
    mat4 projection;
};

layout(location = 0) out struct {
    vec4 color;
} vOut;

void main(){
    vOut.color = color;
    gl_PointSize = pointSize;
    gl_Position = projection * view * model * position;
}