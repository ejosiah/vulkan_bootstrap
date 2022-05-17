#version 450

layout (vertices = 3) out;

layout(push_constant) uniform Levels {
    int outerLevels[3];
    int innerLevel;
};

void main(){
    if(gl_InvocationID == 0){
        gl_TessLevelOuter[0] = outerLevels[0];
        gl_TessLevelOuter[1] = outerLevels[1];
        gl_TessLevelOuter[2] = outerLevels[2];
        gl_TessLevelInner[0] = innerLevel;
    }
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}