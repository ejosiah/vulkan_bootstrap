#version 460 core
#define PI 3.14159265358979

layout(set = 0, binding = 0) uniform sampler2D smokeField;
layout(set = 1, binding = 0) uniform sampler2D cloudMap;
layout(set = 2, binding = 0) uniform sampler2D vectorField;

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
    fragColor /= (1 + fragColor);
}