#version 460

#extension GL_EXT_ray_tracing : require

struct Particle{
    vec4 position;
    vec4 color;
    vec3 normal;
    vec3 tangent;
    vec3 bitangent;
    vec2 uv;
};

layout(set = 0, binding = 0) uniform accelerationStructureEXT topLevelAS;

layout(set = 1, binding = 0) buffer POINT_MASSES_IN{
    Particle particleIn[];
};

layout(set = 2, binding = 0) buffer POINT_MASS_OUT{
    Particle particleOut[];
};

layout(location = 0) rayPayloadEXT float unused;

void main(){
    uint id = gl_LaunchIDEXT.y * gl_LaunchSizeEXT.x + gl_LaunchIDEXT.x;
    vec3 pos = particleOut[id].position.xyz;
    vec3 prev_pos = particleIn[id].position.xyz;

    vec3 origin = prev_pos;
    vec3 dir = pos - prev_pos;
    float tmin = 0.001;
    float tmax = 1.0;

    traceRayEXT(topLevelAS, gl_RayFlagsOpaqueEXT, 0xFF, 0, 0, 0, origin, tmin, dir, tmax, 0);
}