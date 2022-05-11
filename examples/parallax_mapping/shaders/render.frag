#version 450

layout(set = 0, binding = 0) uniform sampler2D albedoMap;
layout(set = 0, binding = 1) uniform sampler2D normalMap;
layout(set = 0, binding = 2) uniform sampler2D depthMap;

layout(push_constant) uniform Constants {
    layout(offset = 192)
    float heightScale;
    int parallaxEnabled;
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

vec2 parallaxMap(vec2 texCoord, vec3 viewDir){
    if(!bool(parallaxEnabled)) return texCoord;
//    const float minLayers = 8.0;
//    const float maxLayers = 32.0;
//    float t = dot(vec3(0, 0, 1), viewDir);
//    t = clamp(t, 0, 1);
//    float numLayers = mix(maxLayers, minLayers, t);
    const float numLayers = 32;
    float layerDepth = 1/numLayers;
    float currentLayerDepth = 0;

    vec2 p = viewDir.xy * heightScale;
    vec2 deltaTexCoord = p/numLayers;

    vec2 currentTexCoord = texCoord;
    float currentDepthMapValue = texture(depthMap, currentTexCoord).r;

    while(currentLayerDepth < currentDepthMapValue){
        currentLayerDepth += layerDepth;
        currentTexCoord -= deltaTexCoord;
        currentDepthMapValue = texture(depthMap, currentTexCoord).r;
    }
    return currentTexCoord;
}

void main(){

    mat3 tangentSpaceToWorld = mat3(fs_in.tangent, fs_in.bitangent, fs_in.normal);
    mat3 worldToTangentSpace = inverse(tangentSpaceToWorld);
    vec3 tViewPos = worldToTangentSpace * fs_in.viewPos;

    vec2 uv = parallaxMap(fs_in.uv, tViewPos);

    bool outOfRange = any(lessThan(uv, vec2(0))) || any(greaterThan(uv, vec2(1)));

    if(outOfRange) discard;

    vec3 N = 2 * texture(normalMap, uv).xyz - 1;
    N = normalize(tangentSpaceToWorld * N);

    vec3 viewDir = fs_in.viewPos - fs_in.worldPos;
    vec3 E = normalize(viewDir);
    vec3 L = E;

    vec3 albedo = texture(albedoMap, uv).rgb;
    vec3 color = max(0, dot(N, L)) * albedo;

    fragColor = vec4(color, 1);
}