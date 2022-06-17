#version 450

layout(location = 0) in vec2 velocity;

layout(location = 0) out vec4 fragColor;

void main(){
    float magintude = length(velocity);
    if(isnan(magintude) || magintude < 0.0001) discard;
    fragColor = vec4(1);
}