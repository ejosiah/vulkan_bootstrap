#version 460

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tanget;
layout(location = 3) in vec3 bitangent;
layout(location = 4) in vec3 color;
layout(location = 5) in vec2 uv;
layout(location = 6) in uvec4 boneIds0;
layout(location = 7) in uvec4 boneIds1;
layout(location = 8) in vec4 weights0;
layout(location = 9) in vec4 weights1;

layout(set = 0, binding = 0) buffer LOCAL_XFORMS{
    mat4 gBones[];
};

layout(push_constant) uniform MVP{
    mat4 model;
    mat4 view;
    mat4 projection;
};

void main(){
    mat4 boneTransform = gBones[boneIds0[0]] * weights0[0];
    boneTransform += gBones[boneIds0[1]] * weights0[1];
    boneTransform += gBones[boneIds0[2]] * weights0[2];
    boneTransform += gBones[boneIds0[3]] * weights0[3];

    boneTransform += gBones[boneIds1[0]] * weights1[0];
    boneTransform += gBones[boneIds1[1]] * weights1[1];
    boneTransform += gBones[boneIds1[2]] * weights1[2];
    boneTransform += gBones[boneIds1[3]] * weights1[3];

    vec4 worldPos = model * boneTransform * position;
    gl_Position = projection * view * worldPos;
}