#pragma once

#include "camera_base.h"

struct FirstPersonSpectatorCameraSettings : public BaseCameraSettings{
};

class SpectatorCameraController : public BaseCameraController {
public:
    SpectatorCameraController(InputManager& inputManager, const FirstPersonSpectatorCameraSettings& settings = {});

    void update(float elapsedTime) override;

    void rotate(float headingDegrees, float pitchDegrees, float rollDegrees) override;
};

class FirstPersonCameraController : public SpectatorCameraController {
public:
    FirstPersonCameraController(InputManager& inputManager, const FirstPersonSpectatorCameraSettings& settings = {});

    void move(float dx, float dy, float dz) override;
};