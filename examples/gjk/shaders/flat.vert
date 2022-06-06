#version 450

layout(location = 0) in vec3 position;

layout(push_constant) uniform UBO{
    mat4 model;
    mat4 view;
    mat4 projection;
};

void main(){
    gl_PointSize = 5.0;
    gl_Position = projection * view * model * vec4(position, 1);
}