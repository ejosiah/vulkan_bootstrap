#version 460

layout(location = 0) in vec4 position;

layout(push_constant) uniform MVP{
    mat4 model;
    mat4 view;
    mat4 projection;
    vec3 lightColor;
};

layout(location = 0) out vec3 vLightColor;

void main(){
    vLightColor = lightColor;
    gl_Position = projection * view * model * position;
}