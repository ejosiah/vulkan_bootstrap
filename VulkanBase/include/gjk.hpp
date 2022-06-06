#pragma once

#include <array>
#include <glm/glm.hpp>
#include <vector>
#include "math/math.hpp"

struct Point{
    glm::vec3 xyz{0}, pointA{0}, pointB{0};

    bool operator==(const Point& rhs) const {
        return (pointA == rhs.pointA) && (pointB == rhs.pointB) && (xyz == rhs.xyz);
    }
};

/**
 * Expanding Polytope Algorithm (EPA)
 */
class EPA{
public:
    static glm::vec3 normalDirection(const Tri& tri, const std::vector<Point>& points);

    static float signedDistanceToTriangle(const Tri& tri, const glm::vec3& point, const std::vector<Point>& points);

    static int closestTriangle(const std::vector<Tri>& triangles, const std::vector<Point>& points);

    static bool hasPoint(const glm::vec3& w, const std::vector<Tri>& triangles, const std::vector<Point>& points);

    static int removeTrianglesFacingPoint(const glm::vec3& point, std::vector<Tri>& triangles, const std::vector<Point>& points);

    static void findDanglingEdges(std::vector<Edge>& danglingEdges, const std::vector<Tri>& triangles);

    template<typename BodyA, typename  BodyB>
    static float expand(const BodyA& bodyA, const BodyB& bodyB, float bias, const std::array<Point, 4>& simplexPoints, glm::vec3& pointOnA, glm::vec3& pointOnB, std::vector<uint16_t>& indices, std::vector<glm::vec3>& tetrahedron){
        std::vector<Point> points;
        std::vector<Tri> triangles;
        std::vector<Edge> danglingEdges;

        glm::vec3 center(0);
        for(int i = 0; i < 4; i++){
            points.push_back(simplexPoints[i]);
            center += simplexPoints[i].xyz;
        }
        center *= 0.25f;

        // build the triangles
        bool inverted = false;
        for(int i = 0; i < 4; i++){
            int j = (i + 1) % 4;
            int k = (i + 2) % 4;
            Tri tri{};
            tri.a = i;
            tri.b = j;
            tri.c = k;

            int unusedPoint = (i + 3) % 4;
            auto dist = signedDistanceToTriangle(tri, points[unusedPoint].xyz, points);

            // The unused point is always on the negative/inside of the triangle .. make sure the normal points away
            if(dist > 0.0f){
                inverted = true;
                std::swap(tri.a, tri.b);
            }

            triangles.push_back(tri);
        }

        if(!inverted){
            std::swap(triangles[0].a, triangles[0].b);
        }

        // expand the simplex to find the closest face of the CSO to the origin'
        while(true){
            const auto idx = closestTriangle(triangles, points);
//            spdlog::info("closest triangle {}", idx);
            auto normal = normalDirection(triangles[idx], points);

            const auto newPoint = GJK::support(bodyA, bodyB, normal, bias);

            // if w already exists, then just stop, because it means we can't expand any further
            if(hasPoint(newPoint.xyz, triangles, points)){
                break;
            }

            float dist = signedDistanceToTriangle(triangles[idx], newPoint.xyz, points);
            if(dist <= 0){
                break; // can't expand
            }

            const auto newIdx = static_cast<int>(points.size());
            points.push_back(newPoint);

            // remove triangles that face this point
//        spdlog::info("newPoint: {}", newPoint.xyz);
            int numRemoved = removeTrianglesFacingPoint(newPoint.xyz, triangles, points);
            if(numRemoved == 0){
                break;
            }

            // find dangling edges
            danglingEdges.clear();
            findDanglingEdges(danglingEdges, triangles);
            if(danglingEdges.empty()){
                break;
            }

            // In theory the edges should be a proper CCW order
            // so we only need to add the new point as 'a' in order
            // to create new triangles that face away from origin
            for(auto & edge : danglingEdges){
                Tri triangle{newIdx, edge.b, edge.a};

                dist = signedDistanceToTriangle(triangle, center, points);
                if(dist > 0.0f){
                    std::swap(triangle.b, triangle.c);
                }
                triangles.push_back(triangle);
            }

        }

        // Get the projection of the origin on the closest triangle
        const auto idx = closestTriangle(triangles, points);
        const auto& tri = triangles[idx];
        auto ptA_w = points[tri.a].xyz;
        auto ptB_w = points[tri.b].xyz;
        auto ptC_w = points[tri.c].xyz;
        auto lambdas = barycentricCoordinates(ptA_w, ptB_w, ptC_w, glm::vec3(0));

        // GEt the point on shape A
        auto ptA_a = points[tri.a].pointA;
        auto ptB_a = points[tri.b].pointA;
        auto ptC_a = points[tri.c].pointA;
        pointOnA = ptA_a * lambdas[0] + ptB_a * lambdas[1] + ptC_a * lambdas[2];

        // Get the point on B
        auto ptA_b = points[tri.a].pointB;
        auto ptB_b = points[tri.b].pointB;
        auto ptC_b = points[tri.c].pointB;
        pointOnB = ptA_b * lambdas[0] + ptB_b * lambdas[1] + ptC_b * lambdas[2];

        // return the penetration distance
        auto delta = pointOnB - pointOnA;

        tetrahedron.clear();
        for(auto& p : points){
            tetrahedron.push_back(p.xyz);
        }

        indices.clear();
        for(auto& tri : triangles){
            indices.push_back(tri.a);
            indices.push_back(tri.b);
            indices.push_back(tri.c);
        }

        return glm::length(delta);
    }
};

/**
 * Gilbert-Johnson-Keerthi (GJK) convex hull intersection algorithm
 */
class GJK{
public:
    template<typename BodyA, typename BodyB>
    static Point support(const BodyA& bodyA, const BodyB& bodyB, glm::vec3 dir, float bias = 0.0f){
        dir = glm::normalize(dir);

        if(glm::any(isnan(dir))){
            assert(false);
        }

        Point point{};

        // Find the point furthest in direction dir
        point.pointA = bodyA.support(dir, bodyA.position, bodyA.orientation, bias);
        dir *= -1;

        // Find the point furthest in opposite direction
        point.pointB = bodyB.support(dir, bodyB.position, bodyB.orientation, bias);

        // Return the point, in the minkowski sum, furthest in the direction
        point.xyz = point.pointA - point.pointB;
        return point;
    }

    static bool simplexSignedVolumes(Point* points, int num, glm::vec3& newDir, glm::vec4& lambdasOut);

    static bool hasPoint(const std::array<Point, 4>& simplexPoints, const Point& newPoint);

    static void sortValids(std::array<Point, 4>& simplexPoints, glm::vec4& lambdas);

    static int numValids(const glm::vec4& lambdas);

    template<typename BodyA, typename BodyB>
    static bool doesIntersect(const BodyA& bodyA, const BodyB& bodyB, float bias, glm::vec3& pointOnA
                              , glm::vec3& pointOnB, std::vector<glm::vec3>& tetrahedron, std::vector<uint16_t>& indices){
        const glm::vec3 origin(0);

        int numPoints = 1;
        std::array<Point, 4> simplexPoints{};
        simplexPoints[0] = support(bodyA, bodyB, glm::vec3(1));

        float closestDist = 1E10f;
        bool doesContainOrigin = false;
        auto newDir = -simplexPoints[0].xyz;
        static int noIntersect = 0;
        do{
            noIntersect++;
            // Get the new point to check on
            auto newPoint = support(bodyA, bodyB, newDir);

            // if the new point is the same as a previous point, then we can't expand any further
            if(hasPoint(simplexPoints, newPoint)){
                break;
            }

            simplexPoints[numPoints] = newPoint;
            numPoints++;

            // if this new point hasn't moved passed the origin, then the
            // origin cannot be in the set. And therefore there is no collision
            if(glm::dot(newDir, newPoint.xyz - origin) < 0.0f){
                break;
            }

            glm::vec4 lambdas;
            doesContainOrigin = simplexSignedVolumes(simplexPoints.data(), numPoints, newDir, lambdas);
            if(doesContainOrigin){
                break;
            }

            // Check that the new projection of the origin onto the simplex is closer than the previous
            auto dist = glm::dot(newDir, newDir);
            if(dist >= closestDist){
                break;
            }
            closestDist = dist;

            // use the lambdas that support the new search direction, and invalidate any points that don't support it
            sortValids(simplexPoints, lambdas);
            numPoints = numValids(lambdas);
            doesContainOrigin = (numPoints == 4);

        } while (!doesContainOrigin);

        if(!doesContainOrigin){
            tetrahedron.clear();
            for(auto& p : simplexPoints){
                tetrahedron.push_back(p.xyz);
            }
            return false;
        }

        // check that we have a 3-simplex (EPA expects a tetrahedron
        if(numPoints == 1){
            auto searchDir = simplexPoints[0].xyz * -1.0f;
            auto newPoint = support(bodyA, bodyB, searchDir);
            simplexPoints[numPoints] = newPoint;
            numPoints++;
        }
        if(numPoints == 2){
            auto ab = simplexPoints[1].xyz - simplexPoints[0].xyz;
            glm::vec3 u, v;
            orthonormal(ab, u, v);
            auto newDir = u;
            auto newPoint = support(bodyA, bodyB, newDir);
            simplexPoints[numPoints] = newPoint;
            numPoints++;
        }
        if(numPoints == 3){
            auto ab = simplexPoints[1].xyz - simplexPoints[0].xyz;
            auto ac = simplexPoints[2].xyz - simplexPoints[0].xyz;
            auto normal = glm::cross(ab, ac);

            auto newDir = normal;
            auto newPoint = support(bodyA, bodyB, newDir);
            simplexPoints[numPoints] = newPoint;
            numPoints++;
        }

        // Expand the simplex by the bias amount

        // Get the center point of the simplex
        glm::vec3 avg{0};
        for(const auto& point : simplexPoints){
            avg += point.xyz;
        }
        avg *= 0.25;

        // Now expand the simplex by the bias amount
        for(auto i = 0; i < numPoints; i++){
            auto& pt = simplexPoints[i];

            auto dir = pt.xyz - avg; // ray from "center" to witness point
            dir = glm::normalize(dir);
            pt.pointA += dir * bias;
            pt.pointB += dir * bias;
            pt.xyz = pt.pointA - pt.pointB;
        }
        tetrahedron.clear();
        for(auto& p : simplexPoints){
            tetrahedron.push_back(p.xyz);
        }

//        FIXME infinite loop for cube at glm::vec3(0.5) and sphere at vec3(0)
        EPA::expand(bodyA, bodyB, bias, simplexPoints, pointOnA, pointOnB, indices, tetrahedron);

        return true;
    }

    template<typename BodyA, typename BodyB>
    static void closestPoint(const BodyA& bodyA, const BodyB& bodyB, glm::vec3& pointOnA, glm::vec3& pointOnB){
        const glm::vec3 origin{0};
        auto closestDist = 1E10f;

        int numPoints = 1;
        std::array<Point, 4> simplexPoints{};
        simplexPoints[0] = support(bodyA, bodyB, glm::vec3(1));

        glm::vec4 lambdas{1, 0, 0, 0};
        auto newDir = -simplexPoints[0].xyz;

        do{
            auto newPoint = support(bodyA, bodyB, newDir);

            // if the new point is the same as the previous point, then we can't expand any further;
            if(hasPoint(simplexPoints, newPoint)){
                break;
            }

            // Add point and get new search direction
            simplexPoints[numPoints] = newPoint;
            numPoints++;

            simplexSignedVolumes(simplexPoints.data(), numPoints, newDir, lambdas);
            sortValids(simplexPoints, lambdas);
            numPoints = numValids(lambdas);

            // Check that the new projection of the origin onto the simplex is closer than the previous
            auto dist = glm::dot(newDir, newDir);
            if(dist >= closestDist){
                break;
            }
            closestDist = dist;
        } while (numPoints < 4);

        pointOnA = glm::vec3{0};
        pointOnB = glm::vec3{0};
        for(auto i = 0; i < 4; i++){
            pointOnA += simplexPoints[i].pointA * lambdas[i];
            pointOnB += simplexPoints[i].pointB * lambdas[i];
        }
    }

};