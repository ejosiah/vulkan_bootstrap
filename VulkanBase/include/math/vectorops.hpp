#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/orthonormalize.hpp>
#include <glm/gtc/matrix_access.hpp>

inline void orthonormal(glm::vec3& a, glm::vec3& b, glm::vec3& c){
    auto n = glm::normalize(a);

    const auto w = (n.z * n.z > 0.9f * 0.9f) ? glm::vec3(1, 0, 0) : glm::vec3(0, 0, 1);

    b = glm::cross(w, n);
    b = glm::normalize(b);

    c = glm::cross(n, b);
    c = glm::normalize(c);
    b = glm::cross(c, n);
    b = glm::normalize(b);
}

template<glm::length_t L, typename T, glm::qualifier Q = glm::defaultp>
constexpr void clear(glm::vec<L, T, Q>& v){
    switch(L){
        case 2:
            v.x = v.y = 0;
            break;
        case 3:
            v.x = v.y = v.z = 0;
            break;
        case 4:
            v.x = v.y = v.z = v.w = 0;
            break;
        default:
            for(auto i = 0; i < L; i++){
                v[i] = 0;
            }
            break;
    }
}