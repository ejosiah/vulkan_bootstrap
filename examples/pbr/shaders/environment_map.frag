#version 450 core

#extension GL_GOOGLE_include_directive : enable

#include "octahedral.glsl"

layout(set = 0, binding = 0) uniform sampler2DArray environmentMap;
layout(set = 0, binding = 1) uniform sampler2DArray irradianceMap;
layout(set = 0, binding = 2) uniform sampler2DArray specularMap;

const int ENV = 0;
const int IRRADIANCE = 1;
const int SPECULAR = 2;

layout(push_constant) uniform Constants{
layout(offset = 192)
    int mapId;
    int selection;
    float lod;
};

layout(location = 0) in vec3 texCoord;

layout(location = 0) out vec4 fragColor;

vec3 color(){
    vec3 dir = texCoord;
    vec3 uv = vec3(octEncode(normalize(dir))  * 0.5 + 0.5, mapId);
    switch(selection){
        case IRRADIANCE:
            return texture(irradianceMap, uv).rgb;
        case SPECULAR:
            return textureLod(specularMap, uv, lod).rgb;
        case ENV:
        default:
            return texture(environmentMap, uv).rgb;
    }
}

void main(){
    fragColor = vec4(color(), 1);
}