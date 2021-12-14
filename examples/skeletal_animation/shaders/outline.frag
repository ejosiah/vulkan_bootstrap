#version 460

layout(location = 0) in struct {
    vec3 position;
    vec3 normal;
    vec3 lightPos;
    vec2 uv;
} v_in;

layout(location = 0) out vec4 fragColor;

void main(){
    fragColor = vec4(0, 0, 0, 1);
}