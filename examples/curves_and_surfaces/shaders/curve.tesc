#version 450

layout(vertices = 4) out;

layout(push_constant) uniform Levels{
    int levels[2];
};

void main(){
    if(gl_InvocationID == 0){
        gl_TessLevelOuter[0] = levels[0];
        gl_TessLevelOuter[1] = levels[1];
    }
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}