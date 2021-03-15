#pragma once

#include "common.h"
#include "InputManager.h"
#include <glm/glm.hpp>

static constexpr float HALF_PI = glm::half_pi<float>();
static constexpr float PI = glm::pi<float>();
static constexpr float TWO_PI = glm::two_pi<float>();

struct Camera{
    glm::mat4 model = glm::mat4(1);
    glm::mat4 view = glm::mat4(1);
    glm::mat4 proj = glm::mat4(1);
};

struct CameraController{
public:
    virtual void update(float time) = 0;

    virtual void processInput(){}
};

struct SphericalCameraController : public CameraController{
public:
    SphericalCameraController(InputManager& inputManager, Camera& camera, float distance)
    : camera(camera)
    , mouse(inputManager.getMouse())
    , distance(distance)
    , theta(HALF_PI * 0.5)
    , phi(0)
    , forward(inputManager.mapToMouse(MouseEvent::MoveCode::WHEEL_UP))
    , backward(inputManager.mapToMouse(MouseEvent::MoveCode::WHEEL_DOWN))
    {
        update(0);
    }

    void update(float time) override {

        updateTheta(mouse.relativePosition.y * 0.01f);
        updatePhi(mouse.relativePosition.x * 0.01f);

        glm::vec3 position = {
                distance * glm::sin(theta) * glm::sin(phi),
                distance * glm::cos(theta),
                distance * glm::sin(theta) * glm::cos(phi)
        };

        camera.view = glm::lookAt(position, glm::vec3(0), {0, 1, 0});
    }

    void updateTheta(float v) {
        theta += v;
        if (theta < 0) theta += TWO_PI;
        if (theta > TWO_PI) theta -= TWO_PI;
    }

    void updatePhi(float v) {
        phi += v;
        if (phi < 0) phi += TWO_PI;
        if (phi > TWO_PI) phi -= TWO_PI;
    }

    virtual void processInput() override{
        if(forward.isPressed()){
            distance -= 0.1;
        }
        if(backward.isPressed()){
            distance += 0.1;
        }
    }

private:
    Camera& camera;
    const Mouse& mouse;
    float distance;
    float theta;
    float phi;
    Action& forward;
    Action& backward;

};