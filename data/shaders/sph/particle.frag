#version 460 core

layout(location = 0) in struct {
    vec4 color;
} vIn;

layout(location = 0) out vec4 fragColor;

void main(){
    fragColor = vIn.color;
}