#include "camera_v2.hpp"
#include "xforms_v2.h"

using namespace jay::camera;

void Camera2::perspective(float fovx, float aspect, float znear, float zfar) {
    _mvp.projection = jay::camera::perspective(glm::radians(_fov), aspect, znear, zfar, _horizontalFov);
    _fov = fovx;
    _aspectRatio = aspect;
    _znear = znear;
    _zfar = zfar;
}

inline void Camera2::zoom(float amount) {
    perspective(amount, _aspectRatio, _znear, _zfar);
}

inline void Camera2::perspective(float aspect) {
    perspective(_fov, aspect, _znear, _zfar);
}

inline void Camera2::position(const glm::vec3 &value) {
    _position = value;
}

inline const glm::vec3 &Camera2::position() const {
    return _position;
}

inline float Camera2::fieldOfView() const {
    return _fov;
}

inline float Camera2::aspectRatio() const {
    return _aspectRatio;
}

inline float Camera2::near() const {
    return _znear;
}

inline float Camera2::far() const {
    return _zfar;
}

bool Camera2::moved() const {
    return _moved;
}
