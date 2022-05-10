// Type your code here, or load an example.
#include <glm/glm.hpp>
#include <fmt/format.h>
#include <glm\glm.hpp>
#include <glm\gtc\packing.hpp>

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>

const float PI = 3.14159265358979323846264338327950288;

float RadicalInverse_VdC(unsigned int bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

glm::vec2 Hammersley(unsigned int i, unsigned int N)
{
    return glm::vec2(float(i) / float(N), RadicalInverse_VdC(i));
}

glm::vec3 ImportanceSampleGGX(glm::vec2 Xi, float roughness, glm::vec3 N)
{
    float a = roughness*roughness;

    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta*cosTheta);

    // from spherical coordinates to cartesian coordinates
    glm::vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    // from tangent-space vector to world-space sample vector
    glm::vec3 up = abs(N.z) < 0.999 ? glm::vec3(0.0, 0.0, 1.0) : glm::vec3(1.0, 0.0, 0.0);
    glm::vec3 tangent = normalize(cross(up, N));
    glm::vec3 bitangent = cross(N, tangent);

    glm::vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float a = roughness;
    float k = (a * a) / 2.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(float roughness, float NoV, float NoL)
{
    float ggx2 = GeometrySchlickGGX(NoV, roughness);
    float ggx1 = GeometrySchlickGGX(NoL, roughness);

    return ggx1 * ggx2;
}

glm::vec2 IntegrateBRDF(float NdotV, float roughness)
{
    glm::vec3 V;
    V.x = sqrt(1.0 - NdotV * NdotV);
    V.y = 0.0;
    V.z = NdotV;

    float A = 0.0;
    float B = 0.0;

    glm::vec3 N = glm::vec3(0.0, 0.0, 1.0);

    constexpr unsigned int samples = 1024;
    for (unsigned int i = 0u; i < samples; ++i)
    {
        glm::vec2 Xi = Hammersley(i, samples);
        glm::vec3 H = ImportanceSampleGGX(Xi, roughness, N);
        glm::vec3 L = normalize(2.0f * dot(V, H) * H - V);

        float NoL = glm::max(L.z, 0.0f);
        float NoH = glm::max(H.z, 0.0f);
        float VoH = glm::max(dot(V, H), 0.0f);
        float NoV = glm::max(dot(N, V), 0.0f);

        if (NoL > 0.0)
        {
            float G = GeometrySmith(roughness, NoV, NoL);

            float G_Vis = (G * VoH) / (NoH * NoV);
            float Fc = pow(1.0 - VoH, 5.0);

            A += (1.0 - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }

    auto res = glm::vec2(A / float(samples), B / float(samples));

    if(any(isnan(res))){
        fmt::print("nan found for NdotV: {} & roughness: {}\n", NdotV, roughness);
    }

    return res;
}

int main(){

    for(float roughness = 0; roughness < 64; roughness++){
        for(float NdotV = 0; NdotV < 64; NdotV++){
            auto brdf = IntegrateBRDF(NdotV, roughness);
        }
    }
    return 0;
}