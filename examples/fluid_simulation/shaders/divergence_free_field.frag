#version 450 core

layout(push_constant) uniform Contants{
    float dt;
    float epsilon;
    float rho;// density;
};

layout(set = 0, binding = 0) uniform sampler2D vectorField;
layout(set = 1, binding = 0) uniform sampler2D pressure;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 velocity_out;

float p(vec2 coord) {
    return texture(pressure, fract(coord)).x;
}

vec2 u(vec2 coord) {
    return texture(vectorField, fract(coord)).xy;
}

vec2 pg(vec2 coord){
    vec3 delta = vec3(1.0/textureSize(pressure, 0), 0);
    vec2 dx = delta.xz;
    vec2 dy = delta.zy;

    float dudx = (p(uv + dx) - p(uv - dx))/(2*dx.x);
    float dudy = (p(uv + dy) - p(uv - dy))/(2*dy.y);

    return vec2(dudx, dudy);
}

void main() {

    velocity_out.xy = u(uv) - pg(uv);
}