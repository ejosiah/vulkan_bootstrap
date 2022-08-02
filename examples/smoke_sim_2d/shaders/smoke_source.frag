#version 450 core

layout(set = 0, binding = 0) uniform sampler2D tempAndDensityField;

layout(push_constant) uniform Constants{
    vec2  location;
    float tempTarget;
    float radius;
    float tempRate;
    float densityRate;
    float decayRate;
    float dt;
};

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 valuesOut;

void main(){
    vec2 values = texture(tempAndDensityField, fract(uv)).xy;
    float temp = values.x;
    float newTemp = temp + (1 - exp(-tempRate * dt)) * (tempTarget - temp);
//    newTemp *= exp(-decayRate * dt);

//    float density = values.y;
//    float newDensity = density + densityRate * dt;
//    newDensity *= 1 - decayRate;

    vec2 d = (location - uv);
    valuesOut.x = newTemp * exp(-dot(d, d)/radius);
//    valuesOut.y = newDensity  * exp(-dot(d, d)/radius);
}