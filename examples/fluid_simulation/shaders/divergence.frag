#version 450 core

layout(set = 0, binding = 0) uniform sampler2D vectorField;

layout(push_constant) uniform Contants{
    float dt;
    float rho;// density;
    float epsilon;
};

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 div;

vec2 u(vec2 coord) {
    return texture(vectorField, fract(coord)).xy;
}

void main(){
    float dx = epsilon;
    float dy = epsilon;
    float alpha = (-2.0 * epsilon * rho / dt);
    float dxdu = u(uv + vec2(dx, 0)).x - u(uv - vec2(dx, 0)).x;
    float dydu = u(uv + vec2(0, dy)).y - u(uv - vec2(0, dy)).y;
    div.x = alpha * (dxdu + dydu);
}