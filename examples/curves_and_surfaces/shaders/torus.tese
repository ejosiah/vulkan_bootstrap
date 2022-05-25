#version 450
#define PI 3.14159265358979
layout(quads, equal_spacing, ccw) in;

layout(push_constant) uniform TESS_LEVELS{
    float outer;
    float inner;
    float maxU;
    float maxV;
    float r;
    float R;
};

layout(location = 0) out struct {
    vec3 normal;
    vec2 uv;
} v_out;


void main(){
    float u = min(gl_TessCoord.x, maxU);
    float v = min(gl_TessCoord.y, maxV);

    float theta = 2 * u * PI;
    float phi = 2 * v * PI;

    vec4 p = vec4(1);
    p.x = (R + r * cos(phi)) * cos(theta);
    p.y = r * sin(phi);
    p.z = (R + r * cos(phi)) * sin(theta);

    vec3 t = vec3(
        -(R + r * cos(phi)) * sin(theta),
        0,
        (R + r * cos(phi)) * cos(theta)
    );


    vec3 b = vec3(
    -r * sin(phi) * cos(theta),
    -r * cos(phi),
    (-r * sin(phi)) * sin(theta)
    );

    vec3 n = normalize(cross(t, b));

    n.x = p.x * (2 - (2 * R)/length(p.xz));
    n.y = 2 * p.y;
    n.z = p.z * (2 - (2 * R)/length(p.xz));
    n = normalize(n);

//    n.x = cos(phi) * cos(theta);
//    n.y = sin(phi);
//    n.z = cos(phi) * sin(theta);

//    n.x = r * R * cos(theta) * cos(phi) + r * r * cos(theta) * cos(phi) * cos(phi);
//    n.y = -r * R * cos(theta) * cos(theta) * cos(phi) * sin(phi) - r * R * sin(theta) * sin(theta) - r * r * cos(phi) * sin(theta) * sin(theta) * sin(phi);
//    n.z = r * R * cos(phi) * sin(theta) * r * r * cos(phi) * cos(phi) * sin(theta);
//    n = normalize(n);
    v_out.normal = -n;
    v_out.uv = vec2(u, v);
    gl_Position = p;
}