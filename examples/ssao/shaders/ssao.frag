#version 460

#extension GL_EXT_scalar_block_layout : enable

layout(constant_id = 0) const int width = 1280;
layout(constant_id = 1) const int height = 720;

layout(set = 0, binding = 0) uniform sampler2D gPosition;
layout(set = 0, binding = 1) uniform sampler2D gNormal;
layout(set = 0, binding = 4) uniform sampler2D noise;

layout(set = 0, binding = 5) buffer SSAO_DATA{
    vec4 samples[];
};


layout(push_constant) uniform Constants{
    mat4 projection;
    int kernelSize;
    float radius;
    float bias;
};

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 fragColor;

void main(){
    vec3 normal = texture(gNormal, uv).xyz;
    vec2 noiseScale = vec2(width, height) * 0.25;
    vec3 randomVec = normalize(texture(noise, uv * noiseScale)).xyz;

    // calculate TBN change of basis from tanget space to view space
    vec3 N = normalize(normal);
    vec3 T = normalize(randomVec - N * dot(randomVec, N));
    vec3 B = cross(N, T);
    mat3 TBN = mat3(T, B, N);

    // iterate over the sample kernel and calculate occlusion factor
    vec3 fragPos = texture(gPosition, uv).xyz;
    float occlusion = 0.0;
    for(int i = 0; i < kernelSize; i++){
        vec3 samplePos = TBN * samples[i].xyz;
        samplePos = fragPos + samplePos * radius;

        // project sample position (to sample texture) (to get position on screen/texture)
        vec4 offset = vec4(samplePos, 1);
        offset = projection * offset; // from view to clip-space
        offset.xyz /= offset.w; // perspective divide;
        offset.xy = offset.xy * 0.5 + 0.5; // from [-1, 1] to [0, 1]

        // get smaple depth
        float sampleDepth = texture(gPosition, offset.xy).z;

        float rangeCheck = smoothstep(0, 1, radius / abs(fragPos.z - sampleDepth));
        occlusion += (sampleDepth >= samplePos.z + bias ? 1 : 0) * rangeCheck;
    }
    occlusion = 1 - (occlusion/kernelSize);

    fragColor = vec4(occlusion);
}