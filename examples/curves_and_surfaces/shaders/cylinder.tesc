#version 450

layout(vertices = 4) out;

layout(push_constant) uniform Contants{
    float outer;
    float inner;
    float maxU;
    float maxV;
    float radius;
    float height;
};

void main(){
    if(gl_InvocationID == 0){
        gl_TessLevelOuter[0] = outer;
        gl_TessLevelOuter[1] = outer;
        gl_TessLevelOuter[2] = outer;
        gl_TessLevelOuter[3] = outer;

        gl_TessLevelInner[0] = inner;
        gl_TessLevelInner[1] = inner;
    }
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}