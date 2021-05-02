#version 460
#extension GL_EXT_ray_tracing : enable

layout(location = 0) rayPayloadInEXT vec3 hitValue;

layout(binding = 5, set = 0) buffer DEBUG_BUFFER{
    vec4 debug[];
};

void main()
{
    uint launchId = gl_LaunchIDEXT.y * gl_LaunchSizeEXT.x + gl_LaunchIDEXT.x;
    debug[launchId] = vec4(0);
    hitValue = vec3(0.0, 0.0, 0.2);
}