#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

inline glm::mat4 qLeft(const glm::quat& q){
    return {
            {q.w, -q.x, -q.y, -q.z},
            {q.x, q.w,  -q.z, q.y},
            {q.y, q.z,  q.w,  -q.x},
            {q.z, -q.y, q.x,  q.w}
    };
}

inline glm::mat4 qRight(const glm::quat& q){
    return {
            {q.w, -q.x, -q.y, -q.z},
            {q.x,  q.w,  q.z, -q.y},
            {q.y, -q.z,  q.w,  q.x},
            {q.z,  q.y, -q.x,  q.w}
    };
}