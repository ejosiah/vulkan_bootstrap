#version 450

layout(location = 0) in vec3 color;
layout(location = 1) out vec2 vector;

layout(location = 0) out vec4 fragColor;

void main(){
    float magintude = length(vector);
    if(isnan(magintude) || magintude < 0.0001) discard;

    fragColor = vec4(color, 1);
}