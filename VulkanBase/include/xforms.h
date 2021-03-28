#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace vkn {

    constexpr glm::mat4 GL_TO_VKN_CLIP(1.0f,  0.0f, 0.0f, 0.0f,
                                      0.0f, -1.0f, 0.0f, 0.0f,
                                      0.0f,  0.0f, 0.5f, 0.0f,
                                      0.0f,  0.0f, 0.5f, 1.0f);

    inline glm::mat4 ortho(float left, float right, float bottom, float top, float near, float far) {
        return GL_TO_VKN_CLIP * glm::ortho(left, right, bottom, top, near, far);
    }

    inline glm::mat4 ortho(float left, float right, float bottom, float top) {
        return GL_TO_VKN_CLIP * glm::ortho(left, right, bottom, top);
    }
    
    inline glm::mat4 perspective(float fovy, float aspect, float zNear, float zFar){
        return GL_TO_VKN_CLIP * glm::perspective(fovy, aspect, zNear, zFar);
    }

}