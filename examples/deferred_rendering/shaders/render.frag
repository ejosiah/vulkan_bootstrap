#version 460

layout(input_attachment_index=0, set=0, binding=0) uniform subpassInput albedo;
layout(input_attachment_index=0, set=0, binding=1) uniform subpassInput normal;
layout(input_attachment_index=0, set=0, binding=2) uniform subpassInput depth;

layout(location = 0) in vec2 vUv;

layout(location = 0) out vec4 fragColor;

void main(){
    fragColor = subpassLoad(albedo);
}