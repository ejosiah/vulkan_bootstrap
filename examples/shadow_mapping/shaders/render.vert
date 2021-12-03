#version 460

layout(set = 0, binding = 0) uniform SCENE_CONSTANTS{
    mat4 model;
    mat4 view;
    mat4 projection;
    mat4 lightSpaceMatrix;
};

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec4 color;
layout(location = 3) in mat4 xform;

layout(location = 0) out struct {
    vec4 lightSpacePos;
    vec3 position;
    vec3 normal;
    vec3 color;
    vec3 lightDir;
    vec3 eyePos;
} v_out;

void main(){
    mat4 worldXform = model * xform;
    vec4 worldPos = worldXform * position;
    v_out.position = worldPos.xyz;
    v_out.normal = mat3(worldXform) * normal;
    v_out.color = color.rgb;
    v_out.lightDir = (inverse(lightSpaceMatrix) * vec4(0, 0, 0, 1)).xyz;
    v_out.eyePos = (view * vec4(0, 0, 0, 1)).xyz;
    v_out.lightSpacePos = lightSpaceMatrix * worldPos;
    gl_Position = projection * view * worldPos;
}