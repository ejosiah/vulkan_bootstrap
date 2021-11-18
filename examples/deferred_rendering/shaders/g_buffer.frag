#version 460 core


layout(set=0, binding=2) uniform sampler2D albedoMap;

layout(push_constant) uniform CamProps{
    layout(offset=192)
    float near;
    float far;
};

float linearizeDepth(float z){
    return (near * far) / (far + near - z * (far - near));
}

layout(location = 0) in struct {
    vec3 position;
    vec3 normal;
    vec3 color;
    vec2 uv;
} v_in;

layout(location = 0) out vec4 abedo;
layout(location = 1) out vec4 normal;
layout(location = 2) out vec4 position;

void main(){
    abedo = texture(albedoMap, v_in.uv);
    normal = vec4(v_in.normal, 1);
    position = vec4(v_in.position, 1);
}