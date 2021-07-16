#pragma once

#include <glm/glm.hpp>

using namespace glm;

#include "../../data/shaders/sph/box_surface.glsl"
#include "../../data/shaders/sph/plane_surface.glsl"

class BoxSurfaceFixture : public ::testing::Test{
protected:

    inline std::tuple<BoxSurface, std::vector<Plane>> createBoxSurface(float lower, float upper){
        return createBoxSurface(glm::vec3(lower), glm::vec3(upper));
    }

    inline std::tuple<BoxSurface, std::vector<Plane>> createBoxSurface(glm::vec3 lower, glm::vec3 upper){
        BoundingBox bounds{lower, upper};
        std::vector<Plane> planes{
                createPlane({1, 0, 0}, bounds.max), createPlane({0, 1, 0}, bounds.max), createPlane({0, 0, 1}, bounds.max),
                createPlane({-1, 0, 0}, bounds.min), createPlane({0, -1, 0}, bounds.min), createPlane({0, 0, -1}, bounds.min),
        };

        BoxSurface surface{ bounds, false};

        return std::make_tuple(surface, planes);
    }
};