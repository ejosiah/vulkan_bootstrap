#version 450 core

layout(push_constant) uniform Constants{
    vec2 force;
    vec2 center;
    float radius;
    float dt;
};

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 du;

void main(){
    vec2 d = (center - uv);
    du.xy = force * exp(-dot(d, d)/radius);
    du.xy /= dt;
}