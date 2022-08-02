#version 460 core
#define PI 3.14159265358979

layout(binding = 0) uniform sampler2D smokeField;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 fragColor;

layout(push_constant) uniform Constants {
    vec3 dye;
};

const float ppm = 1E6;

void main(){
    float density = texture(smokeField, uv).y/ppm;
    vec3 smoke = dye * density;
    fragColor = vec4(smoke, density);
}