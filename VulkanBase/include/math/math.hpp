#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "geometry.hpp"
#include "vectorops.hpp"
#include "matrixops.hpp"
#include "quatOps.hpp"


template<glm::length_t L, typename T, glm::qualifier Q>
constexpr glm::vec<L, T, Q> nansafe(glm::vec<L, T, Q> const& v){
    return glm::any(glm::isnan(v)) ? glm::vec<L, T, Q>(0) : v;
}

template<typename T, glm::qualifier Q>
constexpr glm::qua<T, Q> nansafe(glm::qua<T, Q> const& q){
    return glm::any(glm::isnan(q)) ? glm::qua<T, Q>(1, 0, 0, 0) : q;
}

template<glm::length_t L, typename T, glm::qualifier Q>
constexpr glm::vec<L, T, Q> isvalid(glm::vec<L, T, Q> const& v){
    return !glm::any(glm::isnan(v));
}

template<typename T, glm::qualifier Q>
constexpr bool isvalid(glm::qua<T, Q> const& q){
    return !glm::any(glm::isnan(q));
}
