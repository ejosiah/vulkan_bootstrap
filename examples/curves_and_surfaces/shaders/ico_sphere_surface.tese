#version 450
layout(triangles, equal_spacing, ccw) in;

layout(set = 0, binding = 0) uniform MVP{
    mat4 model;
    mat4 view;
    mat4 projection;
};

layout(push_constant) uniform TESS_LEVELS{
    float outer;
    float inner;
    float maxU;
    float maxV;
    float maxW;
    float radius;
};

layout(location = 0) out struct {
    vec3 normal;
    vec3 worldPos;
    vec3 lightPos;
    vec3 eyePos;
} v_out;

void main(){
    float u = min(gl_TessCoord.x, maxU);
    float v = min(gl_TessCoord.y, maxV);
    float w = min(gl_TessCoord.z, maxW);

    vec3 p0 = gl_in[0].gl_Position.xyz * u;
    vec3 p1 = gl_in[1].gl_Position.xyz * v;
    vec3 p2 = gl_in[2].gl_Position.xyz * w;

    vec3 p = normalize(p0 + p1 + p2) * radius;
    vec3 normal = cross(p1.xyz - p0.xyz, p2.xyz - p0.xyz);

    vec4 position = model * vec4(p, 1);
    v_out.worldPos = position.xyz;
    v_out.normal = -normalize(p);
    v_out.lightPos = (inverse(view) * vec4(0, 0, 0, 1)).xyz;
    v_out.eyePos = v_out.lightPos;

    gl_Position = projection * view * position;

}