#version 460

layout(push_constant) uniform SCENE_CONSTANTS{
    mat4 model;
    mat4 view;
    mat4 projection;
};

layout(location = 0) in vec4 position;

void main(){
    mat4 mvp = projection * view * model;
    gl_Position = mvp * position;
}