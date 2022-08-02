#version 460 core
#define PI 3.14159265358979

layout(binding = 0) uniform sampler2D smokeField;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 fragColor;

void main(){
    float smoke = texture(smokeField, uv).y;
    fragColor = vec4(smoke);
}