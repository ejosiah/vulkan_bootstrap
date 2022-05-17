#version 450

layout (vertices = 4) out;

layout(push_constant) uniform Levels {
    int outerLevels[4];
    int innerLevels[2];
};

void main(){
    if(gl_InvocationID == 0){
        gl_TessLevelOuter[0] = outerLevels[0];
        gl_TessLevelOuter[1] = outerLevels[1];
        gl_TessLevelOuter[2] = outerLevels[2];
        gl_TessLevelOuter[3] = outerLevels[3];
        gl_TessLevelInner[0] = innerLevels[0];
        gl_TessLevelInner[1] = innerLevels[1];
    }
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}