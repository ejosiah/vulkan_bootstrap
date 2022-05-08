#version 450 core

#extension GL_GOOGLE_include_directive : enable

#include "../octahedral.glsl"

layout(set = 0, binding = 0) uniform sampler2D skybox;

layout(location = 0) in vec3 texCoord;

layout(location = 0) out vec4 fragColor;

void main(){
    vec3 dir = texCoord;
    dir.y *= -1;
    vec2 uv = octEncode(normalize(dir))  * 0.5 + 0.5;
    vec3 color = texture(skybox, uv).rgb;
    fragColor = vec4(color, 1);
}