#version 460
#define PI 3.14159265358979

layout(quads, equal_spacing, ccw) in;

layout(push_constant) uniform Contants{
    float outer;
    float inner;
    float maxU;
    float maxV;
    float radius;
    float height;
};

layout(location = 0) out struct {
    vec3 normal;
    vec2 uv;
} v_out;

void main(){
    float u = min(gl_TessCoord.x, maxU);
    float v = min(gl_TessCoord.y, maxV);

    float theta = 2 * PI * u;
    float h = v * height;

    vec3 n = vec3(sin(theta), 0, cos(theta));
    vec4 p = vec4(1);
    p.x = radius * n.x * h;
    p.y = h - height * .5;;
    p.z = radius * n.z * h;

    v_out.normal = n;
    v_out.uv = vec2(u, v);
    gl_Position = p;

}