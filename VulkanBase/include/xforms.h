#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace vkn {

    constexpr glm::mat4 GL_TO_VKN_CLIP(1.0f,  0.0f, 0.0f, 0.0f,
                                      0.0f, -1.0f, 0.0f, 0.0f,
                                      0.0f,  0.0f, 0.5f, 0.0f,
                                      0.0f,  0.0f, 0.5f, 1.0f);

    glm::mat4 perspectiveHFOV(float fovx, float aspect, float znear, float zfar);

    inline glm::mat4 ortho(float left, float right, float bottom, float top, float near, float far) {
        return GL_TO_VKN_CLIP * glm::ortho(left, right, bottom, top, near, far);
    }

    inline glm::mat4 ortho(float left, float right, float bottom, float top) {
        return GL_TO_VKN_CLIP * glm::ortho(left, right, bottom, top);
    }
    
    inline glm::mat4 perspective(float fov, float aspect, float zNear, float zFar, bool horizontalFov = false){
        auto projection = horizontalFov ? perspectiveHFOV(fov, aspect, zNear, zFar) : glm::perspective(fov, aspect, zNear, zFar);
        return GL_TO_VKN_CLIP * projection;
    }

    inline glm::mat4 perspectiveHFOV(float fovx, float aspect, float znear, float zfar){
        float e = 1.0f / tanf(fovx / 2.0f);
        float aspectInv = 1.0f / aspect;
        float fovy = 2.0f * atanf(aspectInv / e);
        float xScale = 1.0f / tanf(0.5f * fovy);
        float yScale = xScale / aspectInv;
        
        glm::mat4 Result;
        Result[0][0] = xScale;
        Result[0][1] = 0.0f;
        Result[0][2] = 0.0f;
        Result[0][3] = 0.0f;

        Result[1][0] = 0.0f;
        Result[1][1] = yScale;
        Result[1][2] = 0.0f;
        Result[1][3] = 0.0f;

        Result[2][0] = 0.0f;
        Result[2][1] = 0.0f;
        Result[2][2] = (zfar + znear) / (znear - zfar);
        Result[2][3] = -1.0f;

        Result[3][0] = 0.0f;
        Result[3][1] = 0.0f;
        Result[3][2] = (2.0f * zfar * znear) / (znear - zfar);
        Result[3][3] = 0.0f;
        
        return Result;
    }

}