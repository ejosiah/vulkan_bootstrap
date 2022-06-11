
vec3 closestPointOnTriangle(vec3 a, vec3 b, vec3 c, vec3 p){
    vec3 ab = b - a;
    vec3 ac = c - a;
    vec3 bc = c - b;

    float snom = dot(p - a, ab);
    float sdenom = dot(p - b, a - b);

    float tnom = dot(p - a, ac);
    float tdenom = dot(p - c, a - c);

    if(snom <= 0 && tnom <= 0) return a;

    float unom = dot(p - b, bc);
    float udenom = dot(p - c, b - c);

    if(sdenom <= 0 && unom <= 0) return b;
    if(tdenom <= 0 && udenom < 0) return c;

    vec3 n = cross(b - a, c - a);
    float vc = dot(n, cross(a - p, b - p));

    if(vc <= 0 && snom >= 0 && sdenom >= 0){
        return a + snom/(snom + sdenom) * ab;
    }

    float va = dot(n, cross(b - p, c - p));

    if(va <= 0 && unom >= 0 && udenom >= 0){
        return b + unom / (unom + udenom) * bc;
    }

    float vb = dot(n, cross(c -p, a - p));

    if(vb <= 0 && tnom >= 0 && tdenom >= 0.0f){
        return a + tnom / (tnom + tdenom) * ac;
    }

    float u = va / (va + vb + vc);
    float v = vb / (va + vb + vc);
    float w = 1 - u - v;
    return u * a + v * b + w * c;
}