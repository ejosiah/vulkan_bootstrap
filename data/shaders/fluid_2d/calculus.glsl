
float dFdx(vec2 uv, sampler2D fSampler){
    vec2 dx = vec2(1.0/textureSize(fSampler, 0).x, 0);
    return (texture(fSampler, uv + dx) - texture(fSampler, uv - dx)).x / (2 * dx.x);
}

float dFdy(vec2 uv, sampler2D fSampler){
    vec2 dy = vec2(0, 1.0/textureSize(fSampler, 0).y);
    return (texture(fSampler, uv + dy) - texture(fSampler, uv - dy)).y / (2 * dy.y);
}

float div(vec2 uv, sampler2D fSampler){
    return dFdx(uv, fSampler) + dFdy(uv, fSampler);
}

vec2 grad(vec2 uv, sampler2D fSampler){
    return vec2(dFdx(uv, fSampler), dFdy(uv, fSampler));
}