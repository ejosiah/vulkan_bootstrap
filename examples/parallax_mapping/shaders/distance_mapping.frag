#version 450

layout(set = 0, binding = 0) uniform sampler2D albedoMap;
layout(set = 0, binding = 1) uniform sampler2D normalMap;
layout(set = 0, binding = 3) uniform sampler3D distanceMap;

layout(push_constant) uniform Constants {
    layout(offset = 192)
    vec3 normalizationFactor;
    int numIterations;
    int enabled;
};

layout(location = 0) in struct {
    vec3 worldPos;
    vec3 viewPos;
    vec3 tangent;
    vec3 bitangent;
    vec3 normal;
    vec2 uv;
} fs_in;


layout(location = 0) out vec4 fragColor;

void main(){
    mat3 tangentSpaceToWorld = mat3(fs_in.tangent, fs_in.bitangent, fs_in.normal);
    mat3 worldToTangentSpace = inverse(tangentSpaceToWorld);
    vec3 tViewPos = worldToTangentSpace * fs_in.viewPos;
    vec3 tFragPos = worldToTangentSpace * fs_in.worldPos;
    vec3 tViewDir = normalize(tViewPos - tFragPos);

    vec3 offset = tViewDir * normalizationFactor;
    vec3 texCoord = vec3(fs_in.uv, 1);

    for(int i = 0; i < numIterations; i++){
        float dist = texture(distanceMap, texCoord).r;
        texCoord -= dist * offset;
    }

    vec2 dx = dFdx(fs_in.uv);
    vec2 dy = dFdy(fs_in.uv);
    vec2 uv = texCoord.xy;


    vec3 N = 2 * textureGrad(normalMap, uv, dx, dy).xyz - 1;
    N = normalize(tangentSpaceToWorld * N);

    vec3 viewDir = fs_in.viewPos - fs_in.worldPos;
    vec3 E = normalize(viewDir);
    vec3 L = E;
    vec3 H = normalize(L + E);

    vec3 albedo = textureGrad(albedoMap, uv, dx, dy).rgb;
    vec3 color = max(0, dot(N, L)) * albedo;
    color += pow(max(0, dot(N, H)), 250);

    fragColor = vec4(color, 1);
}