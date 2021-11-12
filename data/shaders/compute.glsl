#version 460 core
#extension GL_EXT_debug_printf : require

#define printf debugPrintfEXT

layout(local_size_x = 32, local_size_y = 32) in;

layout(rgba32f, binding = 0) uniform image2D image;

void main(){
 //   vec2 uv = vec2(gl_LocalInvocationID.xy + vec2(1))/vec2(gl_WorkGroupSize.xy);
    vec2 uv = vec2(gl_GlobalInvocationID.xy + vec2(1))/(vec2(gl_WorkGroupSize.xy) * vec2(gl_NumWorkGroups.xy));
    uv.y = 1 - uv.y;

    vec2 z = gl_GlobalInvocationID.xy * 0.001 - vec2(0.0, 0.4);
    vec2 c = z;

    vec4 color = vec4(0);

    for(int i = 0; i < 50; i++){
        z.x = z.x * z.x - z.y * z.y + c.x;
        z.y = 2.0 * z.x * z.y + c.y;
        if(dot(z, z) > 10){
            color = i * vec4(0.1, 0.15, 0.2, 0.0);
            break;
        }
    }

    imageStore(image, ivec2(gl_GlobalInvocationID.xy), vec4(uv, 0, 1));

    printf("Hello from invocation (%d, %d)!", gl_GlobalInvocationID.x, gl_GlobalInvocationID.y);
}