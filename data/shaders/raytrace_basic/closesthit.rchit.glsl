#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable

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

layout(binding = 3, set = 0) buffer MATERIALS{
  Material m[];
} materials [];

layout(binding = 4, set = 0) buffer MATERIAL_ID{
  int i[];
} matIds [];

layout(binding = 5, set = 0) buffer DEBUG_BUFFER{
  vec4 debug[];
};

layout(location = 0) rayPayloadInEXT vec3 hitValue;
hitAttributeEXT vec3 attribs;

void main()
{
  uint launchId = gl_LaunchIDEXT.y * gl_LaunchSizeEXT.x + gl_LaunchIDEXT.x;
  const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
 // hitValue = barycentricCoords;

  int matId = matIds[gl_InstanceID].i[gl_PrimitiveID];
  vec3 color = materials[gl_InstanceID].m[matId].diffuse;
//  float t = 1/(gl_HitTEXT + 1);
//  //hitValue = vec3(t);
//  if(gl_PrimitiveID == 8446){
//    debug[launchId] = vec4(gl_InstanceID, gl_InstanceCustomIndexEXT, gl_PrimitiveID, 1);
//  }
//  if(gl_PrimitiveID < 8446){
//    color = vec3(1, 0, 0);
//  }
//  if(gl_PrimitiveID > 8446){
//    color = vec3(0, 1, 0);
//  }
//  if(gl_PrimitiveID < 80){
//    color = vec3(0, 0, 1);
//  }
  hitValue = color;
}
