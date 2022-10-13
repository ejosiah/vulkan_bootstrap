#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable

#include "ray_tracing_lang.glsl"
#include "model.glsl"

layout(set = 0, binding = 0) uniform accelerationStructure topLevelAS;

layout(set = 1, binding = 1) buffer MATERIAL_ID {
    int i[];
} matIds [];

layout(set = 1, binding = 2) buffer OBJECT_INSTANCE {
    SceneObject sceneObjs[];
};


layout(set = 2, binding = 0) buffer VERTEX_BUFFER {
    Vertex v[];
} vertices[];

layout(set = 2, binding = 1) buffer INDEX_BUFFER {
    int i[];
} indices[];

layout(set = 2, binding = 2) buffer VETEX_OFFSETS {
    VertexOffsets vo[];
} offsets[];

layout(location = 0) rayPayloadInEXT vec3 hitValue;

layout(shaderRecord) buffer block {
    vec3 color;
};

//vec3 color = vec3(1, 0, 1);

hitAttributeEXT vec3 attribs;

void main()
{
    uint launchId = gl_LaunchIDEXT.y * gl_LaunchSizeEXT.x + gl_LaunchIDEXT.x;

    float u = 1 - attribs.x - attribs.y;
    float v = attribs.x;
    float w = attribs.y;

    vec3 eyes = gl_WorldRayOriginEXT;
    SceneObject sceneObj = sceneObjs[gl_InstanceID];
    int objId = sceneObj.objId;

    VertexOffsets offset = offsets[objId].vo[gl_InstanceCustomIndexEXT];

    ivec3 index = ivec3(
    indices[objId].i[offset.firstIndex + 3 * gl_PrimitiveID + 0],
    indices[objId].i[offset.firstIndex + 3 * gl_PrimitiveID + 1],
    indices[objId].i[offset.firstIndex + 3 * gl_PrimitiveID + 2]
    );

    Vertex v0 = vertices[objId].v[index.x + offset.vertexOffset];
    Vertex v1 = vertices[objId].v[index.y + offset.vertexOffset];
    Vertex v2 = vertices[objId].v[index.z + offset.vertexOffset];

    vec3 normal = v0.normal * u + v1.normal * v + v2.normal * w;
    vec3 worldPos = gl_WorldRayOriginEXT + gl_HitTEXT * gl_WorldRayDirectionEXT;
    vec3 N = normalize(normal);
    vec3 lightPos = eyes;
    vec3 lightDir = lightPos - worldPos;
    float lightDistance = length(lightDir);
    vec3 E = normalize(eyes - worldPos);
    vec3 L = normalize(lightDir);
    vec3 H = normalize(E + L);

    float shininess = 50;
    float attenuation = 1.0;

    vec3 diffuseColor = color * max(0, dot(N, L));
    hitValue = attenuation * diffuseColor;
}
