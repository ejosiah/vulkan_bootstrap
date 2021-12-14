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

layout(location = 0) out struct {
    vec3 position;
    vec3 normal;
    vec3 lightPos;
    vec2 uv;
} v_out;

void main(){
    vec4 sPosition = position;
    sPosition.xyz += normalize(normal) * 0.8;
    mat4 boneTransform = gBones[boneIds0[0]] * weights0[0];
    boneTransform += gBones[boneIds0[1]] * weights0[1];
    boneTransform += gBones[boneIds0[2]] * weights0[2];
    boneTransform += gBones[boneIds0[3]] * weights0[3];

    boneTransform += gBones[boneIds1[0]] * weights1[0];
    boneTransform += gBones[boneIds1[1]] * weights1[1];
    boneTransform += gBones[boneIds1[2]] * weights1[2];
    boneTransform += gBones[boneIds1[3]] * weights1[3];

    vec4 worldPos = model * boneTransform * sPosition;
    vec3 worldNormal = inverse(transpose(mat3(model * boneTransform))) * normal;

    v_out.position = worldPos.xyz;
    v_out.normal = worldNormal;
    v_out.uv = uv;
    v_out.lightPos = (view * vec4(0, 0, 0, 1)).xyz;
    gl_Position = projection * view * worldPos;
}