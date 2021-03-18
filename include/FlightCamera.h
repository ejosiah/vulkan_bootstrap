#pragma once

#include "camera_base.h"

constexpr float DEFAULT_YAW_SPEED = 100.0F;

struct FlightCameraSettings : BaseCameraSettings{
    float  yawSpeed = DEFAULT_YAW_SPEED;
};

class FlightCameraController : public BaseCameraController{
public:
    FlightCameraController(Camera& camera, InputManager& inputManager, const FlightCameraSettings& settings = {});

    void update(float elapsedTime) override;

    void rotate(float headingDegrees, float pitchDegrees, float rollDegrees) override;

private:
    float YawSpeed;
};