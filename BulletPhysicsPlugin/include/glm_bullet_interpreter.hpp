#pragma once

#include <glm/glm.hpp>
#include <btBulletDynamicsCommon.h>

inline btVector3 to_btVector3(const glm::vec3 &v) {
    return btVector3(v.x, v.y, v.z);
}

inline glm::vec3 to_vec3(const btVector3 &btv) {
    return glm::vec3{
            static_cast<float>(btv.getX()),
            static_cast<float>(btv.getY()),
            static_cast<float>(btv.getZ())
    };
}

inline btMatrix3x3 to_btMatrix3x3(const glm::mat3 m) {
    return btMatrix3x3{
            to_btVector3(glm::row(m, 0)),
            to_btVector3(glm::row(m, 1)),
            to_btVector3(glm::row(m, 2))
    };
}

inline glm::mat3 to_mat3(const btMatrix3x3 &btmat3) {
    return glm::mat3{
            to_vec3(btmat3.getColumn(0)),
            to_vec3(btmat3.getColumn(1)),
            to_vec3(btmat3.getColumn(2))
    };
}