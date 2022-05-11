#version 460

#extension GL_GOOGLE_include_directive : enable

#include "octahedral.glsl"
#include "pbr/common.glsl"

struct Light{
    vec4 position;
    vec4 intensity;
};

layout(set = 1, binding = 0) uniform sampler2D albedoMap;
layout(set = 1, binding = 1) uniform sampler2D metalicMap;
layout(set = 1, binding = 2) uniform sampler2D roughnessMap;
layout(set = 1, binding = 3) uniform sampler2D normalMap;
layout(set = 1, binding = 4) uniform sampler2D aoMap;
layout(set = 1, binding = 5) uniform sampler2D displacementMap;

layout(set = 1, binding = 6) uniform sampler2D brdfLUT;

layout(set = 2, binding = 0) uniform sampler2DArray environmentMap;
layout(set = 2, binding = 1) uniform sampler2DArray irradianceMap;
layout(set = 2, binding = 2) uniform sampler2DArray prefilterMap;

layout(push_constant) uniform Constants{
    int numLights;
    int mapId;
    int invertRoughness;
    int normalMapping;
    int parallaxMapping;
    float heightScale;
};

layout(location = 0) in struct {
    vec3 tViewPos;
    vec3 tPos;
    vec2 uv;
    mat3 tbn;
    Light tLights[10];
} fs_in;

layout(location = 0) out vec4 fragColor;
const float preventDivideByZero = 0.0001;

vec2 getTextureCoord(){
    if(false){
        vec3 viewDir = normalize(fs_in.tViewPos - fs_in.tPos);
        vec2 texCoords = fs_in.uv;

        // number of depth layers
        const float minLayers = 8;
        const float maxLayers = 32;
        float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));
        // calculate the size of each layer
        float layerDepth = 1.0 / numLayers;
        // depth of current layer
        float currentLayerDepth = 0.0;
        // the amount to shift the texture coordinates per layer (from vector P)
        vec2 P = viewDir.xy / viewDir.z * heightScale;
        vec2 deltaTexCoords = P / numLayers;

        // get initial values
        vec2  currentTexCoords     = texCoords;
        float currentDepthMapValue = texture(displacementMap, currentTexCoords).r;

        while(currentLayerDepth < currentDepthMapValue)
        {
            // shift texture coordinates along direction of P
            currentTexCoords -= deltaTexCoords;
            // get displacementMap value at current texture coordinates
            currentDepthMapValue = texture(displacementMap, currentTexCoords).r;
            // get depth of next layer
            currentLayerDepth += layerDepth;
        }

        // get texture coordinates before collision (reverse operations)
        vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

        // get depth after and before collision for linear interpolation
        float afterDepth  = currentDepthMapValue - currentLayerDepth;
        float beforeDepth = texture(displacementMap, prevTexCoords).r - currentLayerDepth + layerDepth;

        // interpolation of texture coordinates
        float weight = afterDepth / (afterDepth - beforeDepth);
        vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

        return finalTexCoords;

    }else{
        return fs_in.uv;
    }
}

void main(){
    vec2 st = getTextureCoord();

    vec3 albedo = texture(albedoMap, st).rgb;
    float metalness = texture(metalicMap, st).r;
    float roughness = texture(roughnessMap, st).r;
    roughness = bool(invertRoughness) ? 1 - roughness : roughness;
    float ao = texture(aoMap, st).r;
    vec3 normal = bool(normalMapping) ? 2 * texture(normalMap, st).rgb - 1 : vec3(0, 0, 1);

    vec3 viewDir = fs_in.tViewPos - fs_in.tPos;
    vec3 N = normalize(normal);
    vec3 E = normalize(viewDir);
    vec3 R = reflect(-E, N);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metalness);

    vec3 lightDir = fs_in.tLights[0].position.xyz - fs_in.tPos;
    vec3 L = normalize(lightDir);

    vec3 Lo = vec3(0);
    for(int i = 0; i < numLights; i++){
        // calculate per-light radiance
        vec3 lightDir = fs_in.tLights[i].position.xyz - fs_in.tPos;
        vec3 L = normalize(lightDir);
        vec3 H = normalize(E + L);
        float dist = length(lightDir);
        float attenuation = 1/(dist * dist);
        vec3 radiance = fs_in.tLights[i].intensity.rgb * attenuation;

        // Cook-Torrance BRDF
        float NDF = distributionGGX(N, H, roughness);
        float G = geometrySmith(N, E, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H,E), 0), F0);

        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, E), 0.0) * max(dot(N, L), 0.0) + preventDivideByZero;
        vec3 specular = numerator / denominator;

        // kS is equal to Fresnel
        vec3 kS = F;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        vec3 kD = vec3(1.0) - kS;
        // multiply kD by the inverse metalness such that only non-metals
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - metalness;

        // scale light by NdotL
        float NdotL = max(dot(N, L), 0.0);

        // add to outgoing radiance Lo
        Lo += (kD * albedo / PI + specular) * radiance * NdotL; // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
    }

    // ambient lighting (we now use IBL as the ambient term)
    N = normalize(fs_in.tbn * N);   // object space to world space
    R = normalize(fs_in.tbn * R);   // object space to world space
    E = normalize(fs_in.tbn * E);   // object space to world space
    vec3 F = fresnelSchlickRoughness(max(dot(N, E), 0.0), F0, roughness);

    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metalness;

    vec3 uv = vec3(octEncode(N)  * 0.5 + 0.5, mapId);
    vec3 irradiance = texture(irradianceMap, uv).rgb;
    vec3 diffuse      = irradiance * albedo;

    // sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular part.
    const float MAX_REFLECTION_LOD = 4.0;
    uv = vec3(octEncode(R)  * 0.5 + 0.5, mapId);
    vec3 prefilteredColor = textureLod(prefilterMap, uv,  roughness * MAX_REFLECTION_LOD).rgb;

    vec2 brdf  = texture(brdfLUT, vec2(max(dot(N, E), 0.0), roughness)).rg;
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

    vec3 ambient = (kD * diffuse + specular) * ao;
    vec3 color = ambient + Lo;
    // HDR tonemapping
    color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0/2.2));

    fragColor = vec4(color , 1.0);
}