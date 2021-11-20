#pragma once

#include "camera_v2.hpp"

namespace jay::camera{
    constexpr glm::vec3 WORLD_XAXIS(1.0f, 0.0f, 0.0f);
    constexpr glm::vec3 WORLD_YAXIS(0.0f, 1.0f, 0.0f);
    constexpr glm::vec3 WORLD_ZAXIS(0.0f, 0.0f, 1.0f);
    static constexpr float DEFAULT_ROTATION_SPEED = 0.3f;
    static constexpr float DEFAULT_ZOOM_MAX = 5.0f;
    static constexpr float DEFAULT_ZOOM_MIN = 1.5f;

    static constexpr glm::vec3 DEFAULT_ACCELERATION(4.0f, 4.0f, 4.0f);
    static constexpr glm::vec3 DEFAULT_VELOCITY(1.0f);

    class Movement{
    public:
        friend class Input;
        friend class CharacterInput;
        explicit Movement(Camera2& camera);

        ~Movement();

        virtual void lookAt(const glm::vec3 &eye, const glm::vec3 &target, const glm::vec3 &up);

        virtual void rotateSmoothly(float headingDegrees, float pitchDegrees, float rollDegrees);

        virtual void rotate(float headingDegrees, float pitchDegrees, float rollDegrees);

        virtual void move(float dx, float dy, float dz);

        virtual void move(const glm::vec3 &direction, const glm::vec3 &amount);

        void updatePosition(const glm::vec3& direction, float dt);

        void updateVelocity(const glm::vec3& direction, float dt);

        virtual void positionUpdated();

        virtual void update(float dt);

        [[nodiscard]]
        const Camera2& camera() const;

        void acceleration(glm::vec3 value);

        [[nodiscard]]
        const glm::vec3& acceleration() const;

        void velocity(glm::vec3 value);

        [[nodiscard]]
        const glm::vec3& velocity() const;

        virtual void undoRoll();

        virtual void zoom(float amount);

        void yaw(float amount);

        void pitch(float amount);

        void roll(float amount);

    protected:
        virtual void updateViewMatrix();

    protected:
        Camera2& _camera;
        float _rotationSpeed;
        float _accumPitchDegrees;
        glm::vec3 _acceleration;
        glm::vec3 _currentVelocity;
        glm::vec3 _velocity;
        glm::quat _orientation;
        glm::vec3 _direction;
        float _minZoom;
        float _maxZoom;
        float _yaw{0};
        float _pitch{0};
        float _roll{0};
    };

    class SpectatorMovement : public Movement{
    public:
        explicit SpectatorMovement(Camera2& camera);

        void rotate(float headingDegrees, float pitchDegrees, float rollDegrees) override;

        void update(float dt) override;

    };

    class FirstPersonMovement : public Movement{
    public:
        explicit FirstPersonMovement(Camera2& camera);

        void move(float dx, float dy, float dz) override;
    };

    class OrbitMovement : public Movement{
    public:
        explicit OrbitMovement(Camera2& camera);

        void rotate(float headingDegrees, float pitchDegrees, float rollDegrees) override;

        void move(float dx, float dy, float dz) override;

        void move(const glm::vec3 &direction, const glm::vec3 &amount) override;

        void positionUpdated() override;

        void update(float dt) override;

        void undoRoll() override;

        void zoom(float amount) override;

        void updateSubject(const glm::vec3& position, const glm::quat& orientation = {1, 0, 0, 0});

    protected:
        void updateViewMatrix() override;

    private:
        float _offsetDistance{0};
        float _orbitRollSpeed{0};
        float _zoomAmount{0};
        bool _preferTargetYAxisOrbiting{false};
        glm::vec3 _targetYAxis{0, 1, 0};

        struct {
            glm::vec3 position{0};
            glm::quat orientation{1, 0, 0, 0};
        } _subject{};
    };

}