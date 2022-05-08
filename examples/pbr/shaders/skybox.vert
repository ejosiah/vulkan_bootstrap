#version 460 core

layout(location = 0) in vec3 position;

layout(location = 0) out vec3 texCord;

layout(push_constant) uniform MVP{
    mat4 model;
    mat4 view;
    mat4 projection;
};

void main(){
    texCord = position;
    mat4 mView = mat4(mat3(view * model));
    gl_Position = (projection * mView * vec4(position, 1)).xyww;
}