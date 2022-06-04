#include <array>
#include "math/signedVolume.hpp"
#include "math/math.hpp"
#include <spdlog/spdlog.h>

glm::vec2 SignedVolume::_1D(const glm::vec3 &s1, const glm::vec3 &s2) {
    auto ab = s2 - s1;
    auto ap = glm::vec3(0) - s1;
    auto p0 = nansafe(s1 + ab * glm::dot(ab, ap)/glm::dot(ab, ab));

    // choose the axis with the greatest difference / length
    int idx = 0;
    float mu_max = 0;
    for(int i = 0; i < 3; i++){
        float mu = ab[i];
        if(mu * mu > mu_max * mu_max){
            mu_max = mu;
            idx = i;
        }
    }

    // Project the simplex points and projected origin onto the axis with the greatest length
    const float a = s1[idx];
    const float b = s2[idx];
    const float p = p0[idx];

    // Get the signed distance from a to p and from p to b;
    const float C1 = p - a;
    const float C2 = b - p;

    // if p is between [a, b]
    if((p > a && p < b) || (p > b && p < a)){
        glm::vec2 lambdas{};
        lambdas[0] = C2 / mu_max;
        lambdas[1] = C1 / mu_max;

        return lambdas;
    }

    // if p is on the far side of a
    if((a <= b && p <= a) || (a >= b && p >= a)){
        return glm::vec2(1.0f, 0.0f);
    }

    // p must e on the far side of b
    return glm::vec2(0.0f, 1.0f);
}

glm::vec3 SignedVolume::_2D(const glm::vec3 &s1, const glm::vec3 &s2, const glm::vec3 &s3) {
    auto normal = glm::cross((s2 - s1), (s3 - s1));
    auto p0 = nansafe(normal * glm::dot(s1, normal)/ glm::dot(normal, normal));

    // Find the axis with the greatest projected area
    int idx = 0;
    float area_max = 0;
    for(auto i = 0; i < 3; i++){
        int j = (i + 1) % 3;
        int k = (i + 2) % 3;
        glm::vec2 a{s1[j], s1[k]};
        glm::vec2 b{s2[j], s2[k]};
        glm::vec2 c{s3[j], s3[k]};
        auto ab = b - a;
        auto ac = c - a;

        float area = ab.x * ac.y - ab.y * ac.x;
        if(area * area > area_max * area_max){
            idx = i;
            area_max = area;
        }
    }

    // Project onto the appropriate axis
    int x = (idx + 1) % 3;
    int y = (idx + 2) % 3;
    std::array<glm::vec2, 3> s{};
    s[0] = {s1[x], s1[y]};
    s[1] = {s2[x], s2[y]};
    s[2] = {s3[x], s3[y]};
    glm::vec2 p{ p0[x], p0[y]};

    // Get the sub-area of the triangles formed from the projected origin and the edge
    glm::vec3 areas;
    for(int i = 0; i < 3; i++){
        int j = (i + 1) % 3;
        int k = (i + 2) % 3;

        auto a = p;
        auto b = s[j];
        auto c = s[k];
        auto ab = b - a;
        auto ac = c - a;

        areas[i] = ab.x * ac.y - ab.y * ac.x;
    }

    // if the projected origin is inside the triangle, then return the barycentric points
    if(compareSigns(area_max, areas[0]) && compareSigns(area_max, areas[1]) && compareSigns(area_max, areas[2])){
        auto lambdas = areas / area_max;
        return lambdas;
    }

    // if we made it here, then we need to project onto the edges and determine the closest point
    float dist = 1e10;
    glm::vec3 lambdas{1, 0, 0};
    for(int i = 0; i < 3; i++){
        int k = (i + 1) % 3;
        int l = (i + 2) % 3;

        std::array<glm::vec3, 3> edgesPts{};
        edgesPts[0] = s1;
        edgesPts[1] = s2;
        edgesPts[2] = s3;

        auto lambdaEdge = _1D(edgesPts[k], edgesPts[l]);
        auto pt = edgesPts[k] * lambdaEdge[0] + edgesPts[l] * lambdaEdge[1];
        if(glm::dot(pt, pt) < dist){
            dist = glm::dot(pt, pt);
            lambdas[i] = 0;
            lambdas[k] = lambdaEdge[0];
            lambdas[l] = lambdaEdge[1];
        }
    }

    return lambdas;
}

glm::vec4 SignedVolume::_3D(const glm::vec3 &s1, const glm::vec3 &s2, const glm::vec3 &s3, const glm::vec3 &s4) {
    glm::mat4 m;

    m[0] = glm::vec4(s1.x, s1.y, s1.z, 1.0);
    m[1] = glm::vec4(s2.x, s2.y, s2.z, 1.0);
    m[2] = glm::vec4(s3.x, s3.y, s3.z, 1.0);
    m[3] = glm::vec4(s4.x, s4.y, s4.z, 1.0);

    glm::vec4 C4;
    C4[0] = cofactor(m, 3, 0);
    C4[1] = cofactor(m, 3, 1);
    C4[2] = cofactor(m, 3, 2);
    C4[3] = cofactor(m, 3, 3);

    const float detM = C4[0] + C4[1] + C4[2] + C4[3];


    // if the barycentric coordinates put the origin inside the simplex, then return them;
    if(compareSigns(detM, C4[0]) && compareSigns(detM, C4[1]) && compareSigns(detM, C4[2]) && compareSigns(detM, C4[3])){
        glm::vec4 lambdas = C4 * (1.0f/detM);
        return lambdas;
    }

    // if we got here, then we need to project the origin onto the faces and determine the closest one
    glm::vec4 lambdas{};
    float dist = 1e10;
    for(auto i = 0; i < 4; i++){
        int j = (i + 1) % 4;
        int k = (i + 2) % 4;

        std::array<glm::vec3, 4> facePts{};
        facePts[0] = s1;
        facePts[1] = s2;
        facePts[2] = s3;
        facePts[3] = s4;

        auto lambdaFace = _2D(facePts[i], facePts[j], facePts[k]);
        auto pt = facePts[i] * lambdaFace[0] + facePts[j] * lambdaFace[1] + facePts[k] * lambdaFace[2];
        if(glm::dot(pt, pt) < dist){
            dist = glm::dot(pt, pt);
            lambdas = glm::vec4(0);
            lambdas[i] = lambdaFace[0];
            lambdas[j] = lambdaFace[1];
            lambdas[k] = lambdaFace[2];
        }
    }

    return lambdas;
}

bool SignedVolume::compareSigns(float a, float b){
    if(a == 0 && b == 0){
        return false;
    }
    return glm::sign(a) == glm::sign(b);
};