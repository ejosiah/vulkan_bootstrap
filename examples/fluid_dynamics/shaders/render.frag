#version 450

layout(set = 0, binding = 0) uniform sampler2D image;
layout(location = 0) in vec2 uv;
layout(Location = 0) out vec4 fragColor;

void main(){
    vec3 color = texture(image, uv).rgb;
    fragColor = vec4(color, 1);
}