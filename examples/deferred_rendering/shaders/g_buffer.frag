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
layout(location = 1) out vec3 normal;
layout(location = 2) out float depth;

void main(){
    abedo = texture(albedoMap, v_in.uv);
    normal = v_in.normal;
    depth = linearizeDepth(gl_FragCoord.z)/far;
}