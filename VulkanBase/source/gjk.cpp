#include "gjk.hpp"
#include "math/signedVolume.hpp"
#include <spdlog/spdlog.h>
#include <glm_format.h>

glm::vec3 EPA::normalDirection(const Tri &tri, const std::vector<Point> &points) {
    const auto& a = points[tri.a].xyz;
    const auto& b = points[tri.b].xyz;
    const auto& c = points[tri.c].xyz;

    auto ab = b - a;
    auto ac = c - a;
    auto normal = glm::normalize(glm::cross(ab, ac));
    return normal;
}

float EPA::signedDistanceToTriangle(const Tri &tri, const glm::vec3 &point, const std::vector<Point> &points) {
    const auto normal = normalDirection(tri, points);
    const auto& a = points[tri.a].xyz;
    const auto a2pt = point - a;
    const auto dist = glm::dot(normal, a2pt);
    return dist;
}

int EPA::closestTriangle(const std::vector<Tri> &triangles, const std::vector<Point> &points) {
    auto minDistSqr = 1e10;

    int idx = -1;
    for(int i = 0; i < triangles.size(); i++){
        const auto& tri = triangles[i];
        float dist = signedDistanceToTriangle(tri, glm::vec3(0), points);
        float distSqr = dist * dist;
        if(distSqr < minDistSqr){
            idx = i;
            minDistSqr = distSqr;
        }
    }
    return idx;
}

bool EPA::hasPoint(const glm::vec3 &w, const std::vector<Tri> &triangles, const std::vector<Point> &points) {
    const auto epsilon = 1E-8;
    glm::vec3 delta{0};

    for(auto tri : triangles){
        delta = w - points[tri.a].xyz;
        if(glm::dot(delta, delta) < epsilon){
            return true;
        }

        delta = w - points[tri.b].xyz;
        if(glm::dot(delta, delta) < epsilon){
            return true;
        }

        delta = w - points[tri.c].xyz;
        if(glm::dot(delta, delta) < epsilon){
            return true;
        }
    }
    return false;
}

int
EPA::removeTrianglesFacingPoint(const glm::vec3 &point, std::vector<Tri> &triangles, const std::vector<Point> &points) {
    int numRemoved = 0;
    for(auto i = 0; i < triangles.size(); i++){
        const auto& tri = triangles[i];
        auto dist = signedDistanceToTriangle(tri, point, points);
        if(dist > 0.0f){
            triangles.erase(begin(triangles) + i);
            i--;
            numRemoved++;
        }
    }
    return numRemoved;
}

void EPA::findDanglingEdges(std::vector<Edge> &danglingEdges, const std::vector<Tri> &triangles) {
    danglingEdges.clear();

    for(auto i = 0; i < triangles.size(); i++){
        const auto& tri = triangles[i];

        std::array<Edge, 3> edges{};
        edges[0].a = tri.a;
        edges[0].b = tri.b;

        edges[1].a = tri.b;
        edges[1].b = tri.c;

        edges[2].a = tri.c;
        edges[2].b = tri.a;

        std::array<int, 3> count{0};

        for(auto j = 0; j < triangles.size(); j++){
            if(j == i){
                continue;
            }

            const auto& tri2 = triangles[j];

            std::array<Edge, 3> edges2{};
            edges2[0].a = tri2.a;
            edges2[0].b = tri2.b;

            edges2[1].a = tri2.b;
            edges2[1].b = tri2.c;

            edges2[2].a = tri2.c;
            edges2[2].b = tri2.a;

            for(auto k = 0; k < 3; k++){
                if(edges[k] == edges2[0]){
                    count[k]++;
                }
                if(edges[k] == edges2[1]){
                    count[k]++;
                }
                if(edges[k] == edges2[2]){
                    count[k]++;
                }
            }
        }

        // An edge that isn't shared is dangling
        for(int k = 0; k < 3; k++){
            if(count[k] == 0){
                danglingEdges.push_back(edges[k]);
            }
        }
    }
}

/* ======================================== GJK =============================== */
bool GJK::simplexSignedVolumes(Point *points, const int num, glm::vec3 &newDir, glm::vec4 &lambdasOut) {
    const float epsilonf = 1E-8;
    lambdasOut = glm::vec4(0);

    auto isZero = [](auto v){
        return glm::all(glm::equal(v, decltype(v)(0)));
    };

    bool doesIntersect = false;
    switch (num) {
        default:
        case 2 : {
            auto lambdas = SignedVolume::_1D(points[0].xyz, points[1].xyz);
            glm::vec3 v(0);
            v += points[0].xyz * lambdas[0];
            v += points[1].xyz * lambdas[1];
            newDir = v * -1.0f;
            if(isZero(lambdas)){
                for(int i = 0; i < num; i++){
                    auto point = points[i];
                }
                assert(!isZero(lambdas));
            }
            doesIntersect = (glm::dot(v, v) < epsilonf);
            lambdasOut[0] = lambdas[0];
            lambdasOut[1] = lambdas[1];
        } break;
        case 3: {
            auto lambdas = SignedVolume::_2D(points[0].xyz, points[1].xyz, points[2].xyz);
            glm::vec3 v(0);
            v += points[0].xyz * lambdas[0];
            v += points[1].xyz * lambdas[1];
            v += points[2].xyz * lambdas[2];
            newDir = v * -1.0f;
            if(isZero(lambdas)){
                for(int i = 0; i < num; i++){
                    auto point = points[i];
                    spdlog::info("cso: {}", point.xyz);
                }
                assert(!isZero(lambdas));
            }
            doesIntersect = (glm::dot(v, v) < epsilonf);
            lambdasOut[0] = lambdas[0];
            lambdasOut[1] = lambdas[1];
            lambdasOut[2] = lambdas[2];
        } break;
        case 4: {
            auto lambdas = SignedVolume::_3D(points[0].xyz, points[1].xyz, points[2].xyz, points[3].xyz);
            glm::vec3 v(0);
            v += points[0].xyz * lambdas[0];
            v += points[1].xyz * lambdas[1];
            v += points[2].xyz * lambdas[2];
            v += points[3].xyz * lambdas[3];
            newDir = v * -1.0f;
            if(isZero(lambdas)){
                for(int i = 0; i < num; i++){
                    auto point = points[i];
                }
                assert(!isZero(lambdas));
            }
            doesIntersect = (glm::dot(v, v) < epsilonf);
            lambdasOut[0] = lambdas[0];
            lambdasOut[1] = lambdas[1];
            lambdasOut[2] = lambdas[2];
            lambdasOut[3] = lambdas[3];
        } break;
    }
    return doesIntersect;
}

bool GJK::hasPoint(const std::array<Point, 4>& simplexPoints, const Point &newPoint) {
    constexpr float precision = 1E-6f;
    for(int i = 0; i < 4; i++){
        auto delta = simplexPoints[i].xyz - newPoint.xyz;
        if(glm::dot(delta, delta) < precision * precision){
            return true;
        }
    }
    return false;
}

void GJK::sortValids(std::array<Point, 4> &simplexPoints, glm::vec4 &lambdas) {
    std::array<bool, 4> valids{};
    for(int i = 0; i < 4; i++){
        valids[i] = true;
        if(lambdas[i] == 0){
            valids[i] = false;
        }
    }

    glm::vec4 validLambdas(0);
    int validCount = 0;
    std::array<Point, 4> validPoints{};

    for(int i = 0; i < 4; i++){
        if(valids[i]){
            validPoints[validCount] = simplexPoints[i];
            validLambdas[validCount] = lambdas[i];
            validCount++;
        }
    }

    for(int i = 0; i < 4; i++){
        simplexPoints[i] = validPoints[i];
        lambdas[i] = validLambdas[i];
    }
}

int GJK::numValids(const glm::vec4 &lambdas) {
    int num = 0;
    for(int i = 0; i < 4; i++){
        if(lambdas[i] != 0){
            num++;
        }
    }
    return num;
}