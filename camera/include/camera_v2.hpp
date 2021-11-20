#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace jay::camera{
    static constexpr float DEFAULT_FOVX = 90.0f;
    static constexpr float DEFAULT_ZNEAR = 0.1f;
    static constexpr float DEFAULT_ZFAR = 1000.0f;

    class Camera2{
    public:
        friend class Movement;
        friend class SpectatorMovement;
        friend class FirstPersonMovement;
        friend class OrbitMovement;

        void zoom(float amount);

        void perspective(float fovx, float aspect, float znear, float zfar);

        void perspective(float aspect);

        void position(const glm::vec3& value);

        [[nodiscard]]
        const glm::vec3& position() const;

        [[nodiscard]]
        float fieldOfView() const;

        [[nodiscard]]
        float aspectRatio() const;

        [[nodiscard]]
        float near() const;

        [[nodiscard]]
        float far() const;

        [[nodiscard]]
        bool moved() const;

    private:
        float _fov{DEFAULT_FOVX};
        float _aspectRatio{1};
        float _znear{DEFAULT_ZNEAR};
        float _zfar{DEFAULT_ZFAR};
        bool _horizontalFov;
        glm::vec3 _position{0};
        glm::vec3 _target{0};
        glm::vec3 _xAxis{1, 0, 0};
        glm::vec3 _yAxis{0, 1, 0};
        glm::vec3 _zAxis{0, 0, 1};
        glm::vec3 _viewDir{0, 0, -1};
        bool _moved{false};

        struct {
            glm::mat4 projection{1};
            glm::mat4 view{1};
            glm::mat4 model{1};
        } _mvp{};
    };
}