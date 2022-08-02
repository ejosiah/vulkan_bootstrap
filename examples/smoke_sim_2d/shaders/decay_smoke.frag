#version 460 core
#define PI 3.14159265358979

layout(binding = 0) uniform sampler2D tempAndDensityField;

layout(push_constant) uniform Constants {
    float densityDecayRate;
    float temperatureDecayRate;
    float dt;
};

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 valuesOut;


void main(){
    vec2 values = texture(tempAndDensityField, fract(uv)).xy;
    float temp = values.x;
    float density = values.y;

    valuesOut.x = temp * exp(-temperatureDecayRate * dt);
    valuesOut.y = density * exp(-densityDecayRate * dt);
}