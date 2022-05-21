#version 450
#define PI 3.14159265358979
layout(quads, equal_spacing, ccw) in;

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

    float theta = 2 * u * PI;
    float phi = v * PI;

    vec4 p = vec4(1);
    p.x = radius * cos(theta) * sin(phi);
    p.y = radius * cos(phi);
    p.z = radius * sin(theta) * sin(phi);

    vec4 position = model * p;
    v_out.worldPos = position.xyz;
    v_out.normal = normalize(p.xyz);
    v_out.lightPos = (inverse(view) * vec4(0, 0, 0, 1)).xyz;
    v_out.eyePos = v_out.lightPos;

    gl_Position = projection * view * position;

}