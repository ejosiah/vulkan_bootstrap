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

layout(location = 0) rayPayloadInEXT float unused;

layout(set = 2, binding = 0) buffer POINT_MASS_OUT{
    Particle particleOut[];
};


void main(){
    uint id = gl_LaunchIDEXT.y * gl_LaunchSizeEXT.x + gl_LaunchIDEXT.x;
    particleOut[id].color = vec4(0, 1, 0, 1);
    unused = 1;
}