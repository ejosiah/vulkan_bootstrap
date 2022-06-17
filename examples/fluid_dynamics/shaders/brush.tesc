#version 450

layout(vertices = 2) out;

void main(){
    if(gl_InvocationID == 0){
        gl_TessLevelOuter[0] = 1;
        gl_TessLevelOuter[1] = 64;
    }
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}