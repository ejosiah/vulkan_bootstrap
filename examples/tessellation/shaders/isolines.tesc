#version 450

layout(vertices = 4) out;

layout(push_constant) uniform Levels {
    int outerTessLevels[2];
};

void main(){
    if(gl_InvocationID == 0){
        gl_TessLevelOuter[0] = outerTessLevels[0];
        gl_TessLevelOuter[1] = outerTessLevels[1];
    }

    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}