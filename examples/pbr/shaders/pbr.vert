#version 460 core

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec3 bitangent;
layout(location = 4) in vec3 color;
layout(location = 5) in vec2 uv;

layout(set = 1, binding = 5) uniform sampler2D displacementMap;

struct Light{
    vec4 position;
    vec4 intensity;
};

layout(set = 0, binding = 0) buffer LIGHTS{
    Light lights[];
};

layout(set = 0, binding = 1) uniform MVP{
    mat4 model;
    mat4 view;
    mat4 proj;
};

layout(push_constant) uniform Constants{
    int numLights;
    int mapId;
    int invertRoughness;
    int normalMapping;
    int parallaxMapping;
    float heightScale;
};

layout(location = 0) out struct {
    vec3 tViewPos;
    vec3 tPos;
    vec2 uv;
    mat3 tbn;
    Light tLights[10];
} vs_out;

void main(){
    mat3 nMatrix = transpose(inverse(mat3(model)));
    mat3 tbnToWorldSpace = nMatrix * mat3(tangent, bitangent, normal);
    mat3 worldSpaceTotbn = transpose(tbnToWorldSpace);

    vec4 localPos = position;
    localPos.y += bool(parallaxMapping) ? texture(displacementMap, uv).r * heightScale : 0;

    vec3 worldPos = (model * localPos).xyz;
    vec3 viewPos = (inverse(view) * vec4(0, 0, 0, 1)).xyz;

    vs_out.tViewPos = worldSpaceTotbn * viewPos;
    vs_out.tPos = worldSpaceTotbn * worldPos;
    vs_out.uv = uv;
    vs_out.tbn = tbnToWorldSpace;

    for(int i = 0; i < numLights; i++){
        vs_out.tLights[i].position = vec4(worldSpaceTotbn * lights[i].position.xyz, 1);
        vs_out.tLights[i].intensity = lights[i].intensity;
    }
    gl_Position = proj * view * model * localPos;
}