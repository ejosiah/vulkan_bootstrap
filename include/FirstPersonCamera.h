#pragma once

#include "camera_base.h"

class SpectatorCameraController : public CameraController {
public:
    SpectatorCameraController(Camera& camera, InputManager& inputManager, const CameraSettings& settings = {});

    void update(float elapsedTime) override;

    void rotate(float headingDegrees, float pitchDegrees, float rollDegrees) override;
};

class FirstPersonCameraController : public SpectatorCameraController {
public:
    FirstPersonCameraController(Camera& camera, InputManager& inputManager, const CameraSettings& settings = {});

    void move(float dx, float dy, float dz) override;

};