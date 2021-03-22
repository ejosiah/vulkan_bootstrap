#pragma once

#include "camera_base.h"

constexpr float DEFAULT_YAW_SPEED = 100.0F;

struct FlightCameraSettings : BaseCameraSettings{
    float  yawSpeed = DEFAULT_YAW_SPEED;
};

class FlightCameraController : public BaseCameraController{
public:
    FlightCameraController(const VulkanDevice& device, uint32_t swapChainImageCount, const uint32_t& currentImageIndex,  InputManager& inputManager, const FlightCameraSettings& settings = {});

    void update(float elapsedTime) override;

    void rotate(float headingDegrees, float pitchDegrees, float rollDegrees) override;

private:
    float YawSpeed;
};