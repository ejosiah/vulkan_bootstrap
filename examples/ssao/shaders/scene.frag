#version 460

layout(location = 0) in struct {
    vec3 viewPos;
    vec3 viewNormal;
    vec3 color;
} f_in;

layout(location = 0) out vec3 position;
layout(location = 1) out vec3 normal;
//layout(location = 2) out vec3 color;

void main(){
    position = f_in.viewPos;
    normal = f_in.viewPos;
//    color = vec3(0.95);
}