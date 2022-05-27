#version 450
#define PI 3.14159265358979

layout(quads, equal_spacing, ccw) in;

layout(set = 3, binding = 0) buffer HERMITE_POINTS {
    vec4 points[];
};

layout(set = 3, binding = 0) buffer HERMITE_POINT_INDICES {
    int indices[];
};


layout(push_constant) uniform TESS_LEVELS{
    float outer;
    float inner;
    float maxU;
    float maxV;
    int numPoints;
};

layout(location = 0) out struct {
    vec3 normal;
    vec2 uv;
} v_out;


void initHermiteMetrics(inout float H[4], float u){
    float uu = u * u;
    float uuu = uu * u;
    H[0] = 2 * uu - 3 * uu + 1;
    H[1] = -2 * uuu + 3 * uu;
    H[2] = uuu - 2 * uu + u;
    H[3] = uuu - uu;
}

void interpolatePoint(out float r, out float h){
    float numLines = float(numPoints/4);
    float U = gl_TessCoord.y * numLines;
    int id = int(clamp(0, numLines - 1, floor(U)));
    float u = fract(U);

    vec4 p0 = points[indices[id * 4 + 0]];
    vec4 t0 = points[indices[id * 4 + 1]];
    vec4 p1 = points[indices[id * 4 + 2]];
    vec4 t1 = points[indices[id * 4 + 3]];

    float H[4];
    initHermiteMetrics(H, u);
    vec4 p = H[0] * p0 + H[1] * p1 + H[2] * t0 + H[3] * t1;

    r = p.x;
    h = p.y;
}

void main(){
    float u = min(gl_TessCoord.x, maxU);
    float v = min(gl_TessCoord.y, maxV);

    float theta = 2 * u * PI;


    float r, h;
    interpolatePoint(r, h);

    vec4 p = vec4(1);
    p.x = r * sin(theta);
    p.y = h;
    p.z = r * cos(theta);

    v_out.normal = vec3(1);
    v_out.uv = vec2(u, v);
    gl_Position = p;
}