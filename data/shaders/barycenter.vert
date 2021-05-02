#version 460

layout(location = 0) in vec3 position;

layout(push_constant) uniform MVP {
    mat4 model;
    mat4 view;
    mat4 projection;
};

vec3 color[3] = {
    vec3(1, 0, 0),
    vec3(0, 1, 0),
    vec3(0, 0, 1)
};

layout(location = 0) out vec3 vColor;

void main(){
    vColor = color[gl_VertexIndex];
    gl_Position = projection * view * model * vec4(position, 1);
}