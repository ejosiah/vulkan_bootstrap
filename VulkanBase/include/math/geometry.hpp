#pragma once

#include <vector>
#include <array>
#include <glm/glm.hpp>
#include <algorithm>

struct Tri{
    int a, b, c;
};

struct Edge{
    int a,  b;

    bool operator==(const Edge& rhs) const {
        return (a == rhs.a && b == rhs.b) || (a == rhs.b && b == rhs.a);
    }
};

inline int findPointFurthestInDir(const std::vector<glm::vec3>& points, const glm::vec3& dir){
    int maxId = 0;
    float maxDist = glm::dot(dir, points.front());
    for(int i = 1; i < points.size(); i++){
        auto dist = glm::dot(dir, points[i]);
        if(maxDist < dist){
            maxDist = dist;
            maxId = i;
        }
    }
    return maxId;
}

inline float distanceFromLine(const glm::vec3& a, const glm::vec3& b, const glm::vec3& point){
    auto ab = glm::normalize(b - a);
    auto ray = point - a;
    auto p = ab * glm::dot(ray, ab);
    return glm::length(ray - p);
}

inline glm::vec3 findPointFurthestFromLine(const std::vector<glm::vec3>& points, const glm::vec3& pointA, const glm::vec3& pointB){
    int maxId = 0;
    float maxDist = distanceFromLine(pointA, pointB, points.front());
    for(auto i = 1; i < points.size(); i++){
        auto dist = distanceFromLine(pointA, pointB, points[i]);
        if(dist > maxDist){
            maxDist = dist;
            maxId = i;
        }
    }
    return points[maxId];
}

inline float distanceFromTriangle(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& point){
    auto ab = b - a;
    auto ac = c - a;
    auto n = glm::normalize(glm::cross(ab, ac));

    auto ray = point - a;

    return glm::dot(ray, n);
}

inline glm::vec3 findPointFurthestFromTriangle(const std::vector<glm::vec3>& points, const glm::vec3& a, const glm::vec3& b, const glm::vec3& c){
    int maxId = 0;
    float maxDist = distanceFromTriangle(a, b, c, points.front());
    for(int i = 1; i < points.size(); i++){
        float dist = distanceFromTriangle(a, b, c, points[i]);
        if(dist > maxDist){
            maxDist = dist;
            maxId = i;
        }
    }
    return points[maxId];
}

inline void buildTetrahedron(const std::vector<glm::vec3>& vertices, std::vector<glm::vec3>& hullPoints, std::vector<Tri>& hullTris){
    hullPoints.clear();
    hullTris.clear();

    std::array<glm::vec3, 4> points{};

    auto idx = findPointFurthestInDir( vertices, {1, 0, 0});
    points[0] = vertices[idx];
    idx = findPointFurthestInDir(vertices, -points[0]);
    points[1] = vertices[idx];
    points[2] = findPointFurthestFromLine(vertices, points[0], points[1]);
    points[3] = findPointFurthestFromTriangle(vertices, points[0], points[1], points[2]);

    float dist = distanceFromTriangle(points[0], points[1], points[2], points[3]);
    if(dist > 0.0f){
        std::swap(points[0], points[1]);
    }

    hullPoints.push_back(points[0]);
    hullPoints.push_back(points[1]);
    hullPoints.push_back(points[2]);
    hullPoints.push_back(points[3]);

    Tri tri{0, 1, 2};
    hullTris.push_back(tri);

    tri = Tri{0, 2, 3};
    hullTris.push_back(tri);

    tri = Tri{2, 1, 3};
    hullTris.push_back(tri);

    tri = Tri{1, 0, 3};
    hullTris.push_back(tri);
}

inline glm::vec3 barycentricCoordinates(glm::vec3 s1, glm::vec3 s2, glm::vec3 s3, const glm::vec3& pt){
    s1 = s1 - pt;
    s2 = s2 - pt;
    s3 = s3 - pt;

    auto normal = glm::cross(s2 - s1, s3 - s1);
    auto p0 = normal * glm::dot(s1, normal)/glm::dot(normal, normal);

    auto idx = 0;
    auto area_max = 0.0f;

    for(int i = 0; i < 3; i++){
        int j = (i + 1) % 3;
        int k = (i + 2) % 3;

        auto a = glm::vec2(s1[j], s1[k]);
        auto b = glm::vec2(s2[j], s2[k]);
        auto c = glm::vec2(s3[j], s3[k]);
        auto ab = b - a;
        auto ac = c - a;

        auto area = ab.x * ac.y - ab.y * ac.x;
        if(area * area > area_max * area_max){
            idx = i;
            area_max = area;
        }
    }

    // Project onto the appropriate axis
    auto x = (idx + 1) % 3;
    auto y = (idx + 2) % 3;
    std::array<glm::vec2, 3> s{};
    s[0] = {s1[x], s1[y]};
    s[1] = {s2[x], s2[y]};
    s[2] = {s3[x], s3[y]};
    glm::vec2 p{ p0[x], p0[y]};

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

    auto lambdas = areas/area_max;

    if(glm::any(glm::isinf(lambdas)) || glm::any(glm::isnan(lambdas))){
        lambdas = {1, 0, 0};
    }

    return lambdas;
}

inline glm::vec3 barycentricCoordinates0(glm::vec3 s1, glm::vec3 s2, glm::vec3 s3, const glm::vec3& point){
    // TODO use method in book
    auto v0 = s2 - s1;
    auto v1 = s3 - s1;
    auto v2 = point - s1;

    auto d00 = glm::dot(v0, v0);
    auto d01 = glm::dot(v0, v1);
    auto d11 = glm::dot(v1, v1);
    auto d20 = glm::dot(v2, v0);
    auto d21 = glm::dot(v2, v1);
    auto denum = d00 * d11 - d01 * d01;

    auto v = (d11 * d20 - d01 * d21 ) / denum;
    auto w = (d00 * d21 - d01 * d20) / denum;
    auto u = 1.0f - v - w;

    return glm::vec3(u, v, w);
}