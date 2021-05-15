struct RayTracePayload{
    vec3 hitValue;
    vec3 bgColor;
};

void orthonormalBasis(vec3 n, out vec3 x, out vec3 y) {
    float s = n.z >= 0.0 ? 1.0 : -1.0;
    float a = -1.0 / (s + n.z);
    float b = n.x * n.y * a;
    x = vec3(1.0 + s * n.x * n.x * a, s * b, -s * n.x);
    y = vec3(b, s + n.y * n.y * a, -n.y);
}

vec3 checkerboard(vec2 uv)
{
    return vec3(mod(floor(uv.x * 4.0) + floor(uv.y * 4.0), 2.0) < 1.0 ? 1.0 : 0.4);
}