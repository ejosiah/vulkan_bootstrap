#version 460 core

layout(binding = 1) uniform sampler2D glyph;

layout(binding = 2) uniform COLOR{
    vec3 color;
};

layout(location = 0) in vec2 vUv;
layout(location = 0) out vec4 fragColor;

void main(){
    float alpha = texture(glyph, vUv).r;
    fragColor = vec4(color, alpha);
}