#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable

struct Material{
  vec3 diffuse;
  vec3 ambient;
  vec3 specular;
  vec3 emission;
  vec3 transmittance;
  float shininess;
  float ior;
  float opacity;
  float illum;
};

struct VertexOffsets{
  int firstIndex;
  int vertexOffset;
  int material;
  int padding1;
};

struct Vertex{
  vec3 position;
  vec3 color;
  vec3 normal;
  vec3 tangent;
  vec3 bitangent;
  vec2 uv;
};

struct SceneObject{
  mat4 xform;
  mat4 xformIT;
  int objId;
  int padding0;
  int padding1;
  int padding2;
};

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;

layout(binding = 0, set = 1) buffer MATERIALS {
  Material m[];
} materials [];

layout(binding = 1, set = 1) buffer MATERIAL_ID {
  int i[];
} matIds [];

layout(binding = 2, set = 1) buffer OBJECT_INSTANCE {
  SceneObject sceneObjs[];  // TODO gl_InstanceCustomIndexEXT
}; // TODO per instance

layout(binding = 0, set = 2) buffer VERTEX_BUFFER {
  Vertex v[];   // TODO VertexOffsets.vertexOffset + i[VertexOffsets.firstIndex + gl_PrimitiveID]
} vertices[]; // TODO use object_id

layout(binding = 1, set = 2) buffer INDEX_BUFFER {
  int i[]; // TODO VertexOffsets.firstIndex + gl_PrimitiveID
} indices[]; // TODO use object_id

layout(binding = 2, set = 2) buffer VETEX_OFFSETS {
  VertexOffsets vo[]; // TODO use gl_InstanceCustomIndexEXT
} offsets[];  // TODO use object_id



layout(location = 0) rayPayloadInEXT vec3 hitValue;

layout(location = 1) rayPayloadEXT bool isShadow;

hitAttributeEXT vec3 attribs;

const vec3 colors[5] = {
  vec3(1, 0, 0),
  vec3(0, 1, 0),
  vec3(0, 0, 1),
  vec3(1, 1, 0),
  vec3(0, 1, 1)
};

void main()
{
  uint launchId = gl_LaunchIDEXT.y * gl_LaunchSizeEXT.x + gl_LaunchIDEXT.x;

  const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
  float u = 1 - attribs.x - attribs.y;
  float v = attribs.x;
  float w = attribs.y;

  vec3 eyes = gl_WorldRayOriginEXT;
  SceneObject sceneObj = sceneObjs[gl_InstanceID];
  int objId = sceneObj.objId;
 // if(gl_InstanceID > 0) objId = 1;

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
  //vec3 worldPos = v0.position * u + v1.position * v + v2.position * w;
  vec3 worldPos = gl_WorldRayOriginEXT + gl_HitTEXT * gl_WorldRayDirectionEXT;
  vec3 N = normalize(normal);
  vec3 lightPos = eyes;
 // vec3 lightPos = vec3(0, 10, 0);
  vec3 lightDir = lightPos - worldPos;
  float lightDistance = length(lightDir);
  vec3 E = normalize(eyes - worldPos);
  vec3 L = normalize(lightDir);
  vec3 H = normalize(E + L);
//  vec3 L = vec3(0, 1, 0);
  int matId = matIds[objId].i[gl_PrimitiveID + offset.material];
 // vec3 color = materials[gl_InstanceCustomIndexEXT].m[matId].diffuse;

//  vec3 color = materials[gl_InstanceCustomIndexEXT].m[matId].diffuse;
//  float shininess = materials[gl_InstanceCustomIndexEXT].m[matId].shininess;
  Material material = materials[objId].m[matId];
  vec3 color = material.diffuse;
  float shininess = material.shininess;


  float attenuation = 1.0;
  isShadow = false;
  if(dot(N, L) > 0){
    isShadow = true;

    float tMin   = 0.001;
    float tMax   = lightDistance;
    uint flags = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT
    | gl_RayFlagsSkipClosestHitShaderEXT;
    traceRayEXT(topLevelAS, flags, 0XFF, 0, 0, 1, worldPos, tMin, L, tMax, 1);
  }
  vec3 specularColor = vec3(0);
  if(isShadow){
    attenuation = 0.3;
  }else{
    specularColor = material.specular * max(0, pow(dot(N, H), shininess));
  }
  vec3 diffuseColor = color * max(0, dot(N, L));

  hitValue = attenuation * (diffuseColor + specularColor);
  //hitValue = vec3(0.6) * max(0, dot(N, L));
//  vec2 uv = vec2(gl_LaunchIDEXT.xy + uvec2(1))/vec2(gl_LaunchSizeEXT.xy);
 // hitValue = N;
//  if(objId == 1) hitValue = vec3(1, 0, 0);
 // if(gl_InstanceCustomIndexEXT == 0) hitValue = vec3(1, 0, 0);
}
