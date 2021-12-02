#version 460

layout(push_constant) uniform SHADOW_CONSTANTS{
    mat4 lightSpaceMatrix;
};

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec4 color;
layout(location = 3) in mat4 xform;

void main(){
    gl_Position = lightSpaceMatrix * xform * position;
}