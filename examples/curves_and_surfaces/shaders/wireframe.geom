#version 450
#extension GL_GOOGLE_include_directive : enable
#include "wireframe.glsl"

layout(triangles) in;

layout(triangle_strip, max_vertices = 3) out;

layout(set = 0, binding = 0) uniform MVP{
    mat4 model;
    mat4 view;
    mat4 projection;
};

layout(location = 0) in struct {
    vec3 normal;
    vec2 uv;
} v_in[3];


layout(location = 0) out struct {
    vec3 normal;
    vec3 worldPos;
    vec3 lightPos;
    vec3 eyePos;
    vec2 uv;
} v_out;

layout(location = 5) noperspective out vec3 edgeDist;

vec3 calcNormal(vec3 p0, vec3 p1, vec3 p2){
    return normalize(cross(p1 - p0, p2 - p0));
}

void main(){
    vec3 p0 = gl_in[0].gl_Position.xyz;
    vec3 p1 = gl_in[1].gl_Position.xyz;
    vec3 p2 = gl_in[2].gl_Position.xyz;

    vec3 edgeDisComb = edgeDistance(p0, p1, p2);

    bool shouldCalculateNormal = all(equal(vec3(0), v_in[0].normal));
    vec3 normal = shouldCalculateNormal ? calcNormal(p0, p1, p2) : vec3(0);

    for(int i = 0; i < gl_in.length(); i++){
        vec4 position = model * gl_in[i].gl_Position;
        v_out.worldPos = position.xyz;
        v_out.normal = shouldCalculateNormal ? normal : v_in[i].normal;
        v_out.uv = v_in[i].uv;
        v_out.lightPos = (inverse(view) * vec4(0, 0, 0, 1)).xyz;
        v_out.eyePos = v_out.lightPos;
        edgeDist = vec3(0);
        edgeDist[i] = edgeDisComb[i];

        gl_Position = projection * view * position;
        EmitVertex();
    }

    EndPrimitive();

}