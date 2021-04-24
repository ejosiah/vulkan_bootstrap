#version 460 core

layout(points) in;

layout(line_strip, max_vertices = 2) out;

layout(push_constant) uniform CONTANTS {
    mat4 model;
    mat4 view;
    mat4 projection;
    vec4 nColor;
    float nlength;
};

layout(location = 0) in struct {
    vec4 color;
    vec3 normal;
} vIn[];

layout(location = 0) out vec4 vColor;

void main(){
    vColor = nColor;
    vec4 position = gl_in[0].gl_Position;
    gl_Position = projection * view * model * position;
    EmitVertex();

    vColor = nColor;
    position.xyz = position.xyz + vIn[0].normal * nlength;
    gl_Position = projection * view * model * position;
    EmitVertex();
}