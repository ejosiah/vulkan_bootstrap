#pragma once

#include "camera_base.h"

class SpectatorCameraController : public BaseCameraController {
public:
    SpectatorCameraController(Camera& camera, InputManager& inputManager, const BaseCameraSettings& settings = {});

    void update(float elapsedTime) override;

    void rotate(float headingDegrees, float pitchDegrees, float rollDegrees) override;
};

class FirstPersonCameraController : public SpectatorCameraController {
public:
    FirstPersonCameraController(Camera& camera, InputManager& inputManager, const BaseCameraSettings& settings = {});

    void move(float dx, float dy, float dz) override;

};