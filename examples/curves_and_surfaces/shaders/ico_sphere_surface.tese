#version 450
layout(triangles, equal_spacing, ccw) in;

layout(push_constant) uniform TESS_LEVELS{
    float outer;
    float inner;
    float maxU;
    float maxV;
    float maxW;
    float radius;
};

layout(location = 0) out vec3 normal;

void main(){
    float u = min(gl_TessCoord.x, maxU);
    float v = min(gl_TessCoord.y, maxV);
    float w = min(gl_TessCoord.z, maxW);

    vec3 p0 = gl_in[0].gl_Position.xyz * u;
    vec3 p1 = gl_in[1].gl_Position.xyz * v;
    vec3 p2 = gl_in[2].gl_Position.xyz * w;

    vec3 p = normalize(p0 + p1 + p2);
    normal = -p;
    p *= radius;

    gl_Position = vec4(p, 1);

}