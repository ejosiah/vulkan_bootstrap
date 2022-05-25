vec2 deriveUV(vec3 p0, vec3 p1, vec3 p2, vec3 t, vec2 b){
    mat3x2 TB = mat3x2(
        t.x, b.x,
        t.y, b.x,
        t.z, b.z
    );

    vec3 e0 = p1 - p0;
    vec3 e1 = p2 - p0;

    mat3x2 E = mat3x2(
        e0.x, e1.x,
        e0.y, e1.y,
        e0.z, e1.z
    );

    mat2 UV = inverse(TB) * E;

    return row(UV, 0);
}