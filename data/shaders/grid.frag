#version 460 core

layout(location = 0) out vec4 fragColor;

layout(constant_id = 0) const float r = 1;
layout(constant_id = 1) const float g = 1;
layout(constant_id = 2) const float b = 1;

void main(){
    fragColor = vec4(r, g, b, 1);
}