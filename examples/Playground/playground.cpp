// Type your code here, or load an example.
#include <glm/glm.hpp>
#include <fmt/format.h>
#include <glm\glm.hpp>
#include <glm\gtc\packing.hpp>

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "glm_format.h"
#include <array>

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


using namespace glm;

void basisFunctions(float* b, float* db, float t)
{
    float t1 = (1.0 - t);
    float t12 = t1 * t1;
    float t13 = t12 * t1;
    // Bernstein polynomials
    b[0] = t12 * t1;
    b[1] = 3.0 * t12 * t;
    b[2] = 3.0 * t1 * t * t;
    b[3] = t * t * t;
    // Derivatives
    db[0] = -3.0 * t1 * t1;
    db[1] = -6.0 * t * t1 + 3.0 * t12;
    db[2] = -3.0 * t * t + 6.0 * t * t1;
    db[3] = 3.0 * t * t;
}

vec3 C(float u, float v, std::array<vec3, 16>& Ps){
    vec3 p00 = Ps[0];
    vec3 p01 = Ps[1];
    vec3 p02 = Ps[2];
    vec3 p03 = Ps[3];
    vec3 p10 = Ps[4];
    vec3 p11 = Ps[5];
    vec3 p12 = Ps[6];
    vec3 p13 = Ps[7];
    vec3 p20 = Ps[8];
    vec3 p21 = Ps[9];
    vec3 p22 = Ps[10];
    vec3 p23 = Ps[11];
    vec3 p30 = Ps[12];
    vec3 p31 = Ps[13];
    vec3 p32 = Ps[14];
    vec3 p33 = Ps[15];
    // Compute basis functions
    float bu[4], bv[4];// Basis functions for u and
    float dbu[4], dbv[4];// Derivitives for u and v

    basisFunctions(bu, dbu, u);
    basisFunctions(bv, dbv, v);
    // Bezier interpolation
    vec3 p =
            p00*bu[0]*bv[0] + p01*bu[0]*bv[1] + p02*bu[0]*bv[2] +
            p03*bu[0]*bv[3] +
            p10*bu[1]*bv[0] + p11*bu[1]*bv[1] + p12*bu[1]*bv[2] +
            p13*bu[1]*bv[3] +
            p20*bu[2]*bv[0] + p21*bu[2]*bv[1] + p22*bu[2]*bv[2] +
            p23*bu[2]*bv[3] +
            p30*bu[3]*bv[0] + p31*bu[3]*bv[1] + p32*bu[3]*bv[2] +
            p33*bu[3]*bv[3];

    return p;
}

static constexpr std::array<float, 4> bc{1, 3, 3, 1};

float B(int i, float u){
    return bc[i] * pow(1.f - u, 3.f - i) * pow(u, i);
}


vec3 Q(float u, float v, std::array<vec3, 16>& Ps){
    vec3 p{0};
    for(int i = 0; i < 4; i++) {
        fmt::print("[");
        for (int j = 0; j < 4; j++) {
            int ij = i * 4 + j;
            p += B(i, u) * B(j, v) * Ps[ij];
            fmt::print("{}, ", B(i, v) );
        }
        fmt::print("]\n");
    }
    return p;
}

int main(){

    float u = 1;
    float v = 1;
    vec4 U{u*u*u, u*u, u, 1};
    vec4 V{v*v*v, v*v, v, 1};


    mat4 B{
        -1,  3, -3, 1,
         3, -6,  3, 0,
        -3,  3,  0, 0,
         1,  0,  0, 0
    };

//    fmt::print("B:\n");
//    fmt::print("{}\n\n", B);

    auto Bt = transpose(B);

    mat4 Px{
        0, 1, 2, 3,
        0, 1, 2, 3,
        0, 1, 2, 3,
        0, 1, 2, 3
    };

//    fmt::print("Px:\n");
//    fmt::print("{}\n\n", Px);

    mat4 Py{
        0, 0, 0, 0,
        1, 1, 1, 1,
        2, 2, 2, 2,
        3, 3, 3, 3
    };

//    fmt::print("Py:\n");
//    fmt::print("{}\n\n", Py);


    mat4 Pz{
        1, 1, 1, 1,
        1, 2, 2, 1,
        1, 3, 3, 1,
        1, 1, 1, 1
    };

//    fmt::print("Pz:\n");
//    fmt::print("{}\n\n", Pz);


    std::array<vec3, 16> Ps{};

    for(int i = 0; i < 4; i++){
        auto Rx = row(Px, i);
        auto Ry = row(Py, i);
        auto Rz = row(Pz, i);
        for(int j = 0; j < 4; j++){
            int id = i * 4 + j;
            Ps[id].x = Rx[j];
            Ps[id].y = Ry[j];
            Ps[id].z = Rz[j];
//            fmt::print("P[{}, {}] => {}\n", i, j, Ps[id]);
        }
    }

//    for(int i = 0; i < 16; i += 4){
//        fmt::print("[{}, ", Ps[i].x);
//        fmt::print("{}, ", Ps[i+1].x);
//        fmt::print("{}, ", Ps[i+2].x);
//        fmt::print("{}", Ps[i+3].x);
//        fmt::print("]\n");
//    }
//    fmt::print("\n\n");
//    for(int i = 0; i < 16; i += 4){
//        fmt::print("[{}, ", Ps[i].y);
//        fmt::print("{}, ", Ps[i+1].y);
//        fmt::print("{}, ", Ps[i+2].y);
//        fmt::print("{}", Ps[i+3].y);
//        fmt::print("]\n");
//    }
//    fmt::print("\n\n");
//    for(int i = 0; i < 16; i += 4){
//        fmt::print("[{}, ", Ps[i].z);
//        fmt::print("{}, ", Ps[i+1].z);
//        fmt::print("{}, ", Ps[i+2].z);
//        fmt::print("{}", Ps[i+3].z);
//        fmt::print("]\n");
//    }
//    fmt::print("\n\n");

    fmt::print("{}\n\n", B);

    mat4 m(1);
    rotate(m, 0.f, {0, 1, 0});

    auto Qx = U * B * Px * Bt * V;
    auto Qy = U * B * Py * Bt * V;
    auto Qz = U * B * Pz * Bt * V;

    vec3 ps{Qx.w, Qy.z, Qz.w};

//    fmt::print("{}\n", Qx);
//    fmt::print("{}\n", Qy);
//    fmt::print("{}\n\n", Qz);

    auto p = Q(u, v, Ps);
    auto p1 = C(u, v, Ps);
//    fmt::print("{}\n", ps);
//    fmt::print("{}\n", p);
//    fmt::print("{}", p1);
    return 0;
}