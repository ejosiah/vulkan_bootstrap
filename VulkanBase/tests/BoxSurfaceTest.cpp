#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "BoxSurfaceFixture.hpp"
#include <vector>
#include <glm/gtc/epsilon.hpp>
#include <fmt/format.h>
#include <common.h>
#include "glm_format.h"

TEST_F(BoxSurfaceFixture, closestPointWhenPointInsideBox){
    auto [surface, planes] = createBoxSurface(-0.5, 0.5);

    vec3 mid = midPoint(surface.bounds);
    for(auto& p : planes){

        vec3 expectedPointOnSurface = closestPoint(p, mid);
        vec3 point = expectedPointOnSurface - 0.2f * p.normal;

        float closestDist;
        vec3 closestNormal;
        vec3 actualPointOnSurface = closestPoint(surface, point, closestNormal, closestDist);
        ERR_GUARD_VULKAN_TRUE(vectorEquals(expectedPointOnSurface, actualPointOnSurface));
        ERR_GUARD_VULKAN_TRUE(vectorEquals(p.normal, closestNormal));
        ERR_GUARD_VULKAN_FLOAT_EQ(0.2f, closestDist);
    }
}

TEST_F(BoxSurfaceFixture, closestOnSurfaceWhenPointOutsideBox){
    auto [surface, planes] = createBoxSurface(-0.5, 0.5);

    vec3 mid = midPoint(surface.bounds);
    for(auto& p : planes){

        vec3 expectedPointOnSurface = closestPoint(p, mid);
        vec3 point = expectedPointOnSurface + 0.2f * p.normal;

        float closestDist;
        vec3 closestNormal;
        vec3 actualPointOnSurface = closestPoint(surface, point, closestNormal, closestDist);
        ERR_GUARD_VULKAN_TRUE(vectorEquals(expectedPointOnSurface, actualPointOnSurface));
        ERR_GUARD_VULKAN_TRUE(vectorEquals(p.normal, closestNormal));
        ERR_GUARD_VULKAN_FLOAT_EQ(0.2f, closestDist);
    }
}

TEST_F(BoxSurfaceFixture, returnFlipedNormalOnClosestPointIfEnabled){
    auto [surface, planes] = createBoxSurface(-0.5, 0.5);
    surface.normalFlipped = true;
    vec3 mid = midPoint(surface.bounds);
    for(auto& p : planes){

        vec3 expectedPointOnSurface = closestPoint(p, mid);
        vec3 point = expectedPointOnSurface + 0.2f * p.normal;

        float closestDist;
        vec3 closestNormal;
        vec3 actualPointOnSurface = closestPoint(surface, point, closestNormal, closestDist);
        ERR_GUARD_VULKAN_TRUE(vectorEquals(expectedPointOnSurface, actualPointOnSurface));
        ERR_GUARD_VULKAN_TRUE(vectorEquals(-p.normal, closestNormal));
        ERR_GUARD_VULKAN_FLOAT_EQ(0.2f, closestDist);
    }
}

TEST_F(BoxSurfaceFixture, returnFalseIfObjectIsNotPenetratingThroughSurface){
    auto [surface, _] = createBoxSurface({-0.5, 0, -0.5}, {0.5, 2, 0.5 });
    surface.normalFlipped = true;
    float radius = 0.02;
    glm::vec3 point{-0.044, 0.916, -0.084};

    vec3 normal;
    vec3 surfacePoint;
    ERR_GUARD_VULKAN_FALSE(isPenetrating(surface, point, radius, normal, surfacePoint)) << "point should not penetrate through surface";
}