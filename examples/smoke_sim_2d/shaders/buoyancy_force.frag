#version 450 core

layout(set = 0, binding = 0) uniform sampler2D forceField;
layout(set = 1, binding = 0) uniform sampler2D tempAndDensityField;

layout(set = 2, binding = 1) buffer Ubo {
    float ambientTemp;
};

layout(push_constant) uniform Constants {
    vec2 up;
    float tempFactor;
    float densityFactory;
};

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 buoyancy;

vec2 accumForce(vec2 coord){
    return texture(forceField, fract(coord)).xy;
}

void main(){
    vec2 values = texture(tempAndDensityField, fract(uv)).xy;
    float temp = values.x;
    float density = values.x;
    buoyancy.xy =  (-densityFactory * density +  tempFactor * (temp - ambientTemp)) * up;
}