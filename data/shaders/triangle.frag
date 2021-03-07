#version 460 core

layout(set = 0, binding = 1) uniform sampler2D image;

layout(location = 0) in vec3 vColor;
layout(location = 1) in vec2 vUv;
layout(location = 0) out vec4 fragColor;

void main(){
    vec4 texColor = texture(image, vUv).rgba;
    fragColor = vec4(mix(vColor, texColor.rgb, texColor.a), 1.0);
}