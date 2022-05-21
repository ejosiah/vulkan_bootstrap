#version 450

layout(push_constant) uniform MVP{
    mat4 model;
    mat4 view;
    mat4 projection;
//    vec3 center;
//    vec4 selectedColor;
//    float radius;
};

layout(location = 0) in vec3 position;

layout(location = 0) out vec3 color;

void main(){
    color = vec3(1, 0, 0);
//    if(length(position - center) - radius < 0.0001){
//        color = selectedColor;
//    }
    mat4 MVP = projection * view * model;

    gl_PointSize = 5;
    gl_Position = MVP * vec4(position, 1);
}