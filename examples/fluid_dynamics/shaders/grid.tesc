#version 450

layout(vertices = 4) out;

layout(push_constant) uniform GridSize {
    int N;
};

layout(location = 0) in float iOrientation[];

layout(location = 0) out float oOrientation[];
void main(){
    if(gl_InvocationID == 0){
        gl_TessLevelOuter[0] = (N+2);
        gl_TessLevelOuter[1] = (N+2);
    }

    oOrientation[gl_InvocationID] = iOrientation[gl_InvocationID];
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}