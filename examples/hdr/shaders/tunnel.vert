#version 460

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec3 bitangent;
layout(location = 4) in vec4 color;
layout(location = 5) in vec2 uv;

struct Light{
    vec3 position;
    vec3 color;
};

layout(set = 0, binding = 0) buffer LIGHTS{
    Light lights[4];
};

layout(push_constant) uniform MVP{
    mat4 model;
    mat4 view;
    mat4 projection;
};

layout(location = 0) out struct {
    vec3 position;
    vec3 normal;
    vec3 eyes;
    vec2 uv;
    vec3 lightsPos[4];
} v_out;

void main(){
    vec4 worldPos = model * position;
    vec3 worldNormal = inverse(transpose(mat3(model))) * normal;
    vec4 eyes = view * vec4(0, 0, 0, 1);

    v_out.position = worldPos.xyz;
    v_out.normal = worldNormal;
    v_out.eyes = eyes.xyz;
    v_out.uv = uv;
    for(int i = 0; i < 4; i++){
        v_out.lightsPos[i] = lights[i].position;
    }

    gl_Position = projection * view * worldPos;
}