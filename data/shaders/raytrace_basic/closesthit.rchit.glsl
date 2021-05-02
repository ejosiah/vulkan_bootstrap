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

layout(binding = 3, set = 0, scalar) buffer MATERIALS{
  Material materials[];
};

layout(binding = 4, set = 0) buffer MATERIAL_ID{
  int i[];
} matIds[];

layout(location = 0) rayPayloadInEXT vec3 hitValue;
hitAttributeEXT vec3 attribs;

void main()
{
  const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
 // hitValue = barycentricCoords;
  int matId = matIds[0].i[gl_PrimitiveID];
  vec3 color = materials[2].diffuse;
  float t = 1/(gl_HitTEXT + 1);
  //hitValue = vec3(t);
  hitValue = color;
}
