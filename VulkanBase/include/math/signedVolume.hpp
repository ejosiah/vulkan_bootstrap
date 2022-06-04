#include <glm/glm.hpp>

#pragma once

struct SignedVolume{

    static glm::vec2 _1D(const glm::vec3& s1, const glm::vec3& s2);

    static glm::vec3 _2D(const glm::vec3& s1, const glm::vec3& s2, const glm::vec3& s3);

    static glm::vec4 _3D(const glm::vec3& s1, const glm::vec3& s2, const glm::vec3& s3, const glm::vec3& s4);

    static bool compareSigns(float a, float b);

};
