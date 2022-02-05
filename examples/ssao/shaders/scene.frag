#version 460

layout(set=0, binding=2) uniform sampler2D albedoMap;

layout(location = 0) in struct {
    vec3 viewPos;
    vec3 viewNormal;
    vec2 uv;
} f_in;

layout(location = 0) out vec3 position;
layout(location = 1) out vec3 normal;
layout(location = 2) out vec3 color;

void main(){
    position = f_in.viewPos;
    normal = f_in.viewNormal;
    color = texture(albedoMap, f_in.uv).rgb;
}