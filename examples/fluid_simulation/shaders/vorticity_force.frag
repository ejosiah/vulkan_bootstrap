#version 450 core

layout(set = 0, binding = 0) uniform sampler2D vorticityField;
layout(set = 1, binding = 0) uniform sampler2D forceField;

layout(push_constant) uniform Constants{
    float csCale;
};

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 force;

float vort(vec2 coord) {
    return texture(vorticityField, fract(coord)).x;
}

vec2 accumForce(vec2 coord){
    return texture(forceField, fract(coord)).xy;
}

void main(){
    vec3 delta = vec3(1.0/textureSize(vorticityField, 0), 0);
    vec2 dx = delta.xz;
    vec2 dy = delta.zy;


    float dudx = (abs(vort(uv + dx)) - abs(vort(uv - dx)))/(2*dx.x);
    float dudy = (abs(vort(uv + dy)) - abs(vort(uv - dy)))/(2*dy.y);

    vec2 n = vec2(dudx, dudy);

    // safe normalize
    float epsilon = 2.4414e-4;
    float magSqr = max(epsilon, dot(n, n));
    n = n * inversesqrt(magSqr);

    float vc = vort(uv);
    vec2 eps = delta.xy * csCale;
    force.xy = eps * vc * n * vec2(1, -1) + accumForce(uv);
}