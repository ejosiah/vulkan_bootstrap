#version 450

layout(set = 0, binding = 0) uniform sampler2D albedoMap;
layout(set = 0, binding = 1) uniform sampler2D normalMap;
layout(set = 0, binding = 2) uniform sampler2D depthMap;

layout(push_constant) uniform Constants {
    layout(offset = 192)
    float depth;
    int enabled;
    int invert;
};

layout(location = 0) in struct {
    vec3 worldPos;
    vec3 viewPos;
    vec3 tangent;
    vec3 bitangent;
    vec3 normal;
    vec2 uv;
} fs_in;

float find_intersection(vec2 dp, vec2 ds) {
    if(!bool(enabled)) return 0;
    const int linear_steps = 10;
    const int binary_steps = 5;
    float depth_step = 1.0 / linear_steps;
    float size = depth_step;
    float depth = 1.0;
    float best_depth = 1.0;
    for (int i = 0 ; i < linear_steps - 1 ; ++i) {
        depth -= size;
        float t = texture(depthMap, dp + ds * depth).r;
        t = bool(invert) ? 1 - t : t;
        if (depth >= 1.0 - t) best_depth = depth;
    }
    depth = best_depth - size;
    for (int i = 0 ; i < binary_steps ; ++i) {
        size *= 0.5;
        float t = texture(depthMap, dp + ds * depth).r;
        t = bool(invert) ? 1 - t : t;
        if (depth >= 1.0 - t) {
            best_depth = depth;
            depth -= 2 * size;
        }
        depth += size;
    }
    return best_depth;
}

layout(location = 0) out vec4 fragColor;

void main(){
    mat3 tangentSpaceToWorld = mat3(fs_in.tangent, fs_in.bitangent, fs_in.normal);
    mat3 worldToTangentSpace = inverse(tangentSpaceToWorld);
    vec3 tViewPos = worldToTangentSpace * fs_in.viewPos;
    vec3 tFragPos = worldToTangentSpace * fs_in.worldPos;
    vec3 tViewDir = normalize(tViewPos - tFragPos);

    vec2 ds = tViewDir.xy * depth / tViewDir.z;
    vec2 dp = fs_in.uv;
    float dist = find_intersection(dp, ds);
    vec2 uv = dp + dist * ds;

    vec3 N = 2 * texture(normalMap, uv).xyz - 1;
    N = normalize(tangentSpaceToWorld * N);

    vec3 viewDir = fs_in.viewPos - fs_in.worldPos;
    vec3 E = normalize(viewDir);
    vec3 L = E;
    vec3 H = normalize(E + L);
    vec3 albedo = texture(albedoMap, uv).rgb;
    vec3 color = max(0, dot(N, L)) * albedo;
    color += pow(max(0, dot(N, H)), 250);

    fragColor = vec4(color, 1);
}