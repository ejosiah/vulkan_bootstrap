#version 460 core

layout(set = 0, binding = 0) uniform samplerCube skybox;

layout(location = 0) in vec3 texCoord;

layout(location = 0) out vec4 fragColor;

void main(){
    fragColor = texture(skybox, texCoord);
}