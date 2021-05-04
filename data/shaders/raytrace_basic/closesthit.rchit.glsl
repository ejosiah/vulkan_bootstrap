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
};

layout(binding = 0, set = 1) buffer MATERIALS {
  Material m[];
} materials [];

layout(binding = 1, set = 1) buffer MATERIAL_ID {
  int i[];
} matIds [];

layout(binding = 2, set = 1) buffer OBJECT_INSTANCE {
  SceneObject sceneObjs[];  // TODO gl_InstanceCustomeIndexEXT
};

layout(binding = 0, set = 2) buffer VERTEX_BUFFER {
  Vertex v[];   // TODO VertexOffsets.vertexOffset + i[VertexOffsets.firstIndex + gl_PrimitiveID]
} vertices[]; // TODO use object_id

layout(binding = 1, set = 2) buffer INDEX_BUFFER {
  int i[]; // TODO VertexOffsets.firstIndex + gl_PrimitiveID
} indices[]; // TODO use object_id

layout(binding = 2, set = 2) buffer VETEX_OFFSETS {
  VertexOffsets vo[]; // TODO use gl_InstanceID
} offsets[];  // TODO use object_id



layout(location = 0) rayPayloadInEXT vec3 hitValue;
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
  SceneObject sceneObj = sceneObjs[gl_InstanceCustomIndexEXT];
  int objId = sceneObj.objId;

  VertexOffsets offset = offsets[objId].vo[gl_InstanceID];


  ivec3 index = ivec3(
    indices[objId].i[offset.firstIndex + 3 * gl_PrimitiveID + 0],
    indices[objId].i[offset.firstIndex + 3 * gl_PrimitiveID + 1],
    indices[objId].i[offset.firstIndex + 3 * gl_PrimitiveID + 2]
  );

  Vertex v0 = vertices[objId].v[index.x + offset.vertexOffset];
  Vertex v1 = vertices[objId].v[index.y + offset.vertexOffset];
  Vertex v2 = vertices[objId].v[index.z + offset.vertexOffset];

  vec3 normal = v0.normal * u + v1.normal * v + v2.normal * w;
  vec3 worldPos = v0.position * u + v1.position * v + v2.position * w;
  vec3 N = normalize(normal);
  vec3 L = eyes - worldPos;

  int matId = matIds[objId].i[gl_PrimitiveID];
 // vec3 color = materials[gl_InstanceID].m[matId].diffuse;
  vec3 color = materials[gl_InstanceID].m[matId].diffuse;

  hitValue = color * dot(N, L);
  //hitValue = N;
 // hitValue = worldPos;
 // hitValue = colors[gl_InstanceID];

//    if(gl_PrimitiveID == 8446){
//      debug[launchId] = vec4(gl_InstanceID, gl_InstanceCustomIndexEXT, gl_PrimitiveID, 1);
//    }
//  if(gl_InstanceID == 0 && offset.firstIndex != 0) color = vec3(1, 0, 0);
//  if(gl_InstanceID == 1  && offset.firstIndex != 25338) color = vec3(0, 1, 0);
//  if(gl_InstanceID == 2  && offset.firstIndex != 25602) color = vec3(0, 0, 1);
//  if(gl_InstanceID == 3  && offset.firstIndex != 25746) color = vec3(1, 1, 0);
//  if(gl_InstanceID == 4  && offset.firstIndex != 25866) color = vec3(0, 1, 1);
//  hitValue = color;
}
