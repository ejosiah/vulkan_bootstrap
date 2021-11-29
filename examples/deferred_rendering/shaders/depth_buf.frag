#version 460 core

#extension GL_EXT_debug_printf : require
#define printf debugPrintfEXT

layout(set=0, binding=2) uniform sampler2D albedoMap;


layout(location = 0) out vec4 fragColor;

layout(push_constant) uniform CamProps{
    layout(offset=192)
    float near;
    float far;
};

float linearizeDepth(float z){
    return (near * far) / (z * (far - near) - far);
}

layout(location = 0) in struct {
    vec3 position;
    vec3 normal;
    vec3 color;
    vec2 uv;
} v_in;

void main(){
    float depth = linearizeDepth(gl_FragCoord.z)/far;
    vec3 color = texture(albedoMap, v_in.uv).rgb;
    fragColor = vec4(color, 1);
}