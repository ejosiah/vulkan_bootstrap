#version 460 core

layout(local_size_x = 32, local_size_y = 32) in;

layout(rgba32f, binding = 0) uniform image2D image;

void main(){
    vec2 uv = vec2(gl_LocalInvocationID.xy + vec2(1))/vec2(gl_WorkGroupSize.xy);

    imageStore(image, ivec2(gl_GlobalInvocationID.xy), vec4(uv, 0, 1));
}