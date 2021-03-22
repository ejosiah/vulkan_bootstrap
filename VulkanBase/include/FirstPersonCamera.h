#pragma once

#include "camera_base.h"

class SpectatorCameraController : public BaseCameraController {
public:
    SpectatorCameraController(const VulkanDevice& device, uint32_t swapChainImageCount, const uint32_t& currentImageIndex, InputManager& inputManager, const BaseCameraSettings& settings = {});

    void update(float elapsedTime) override;

    void rotate(float headingDegrees, float pitchDegrees, float rollDegrees) override;
};

class FirstPersonCameraController : public SpectatorCameraController {
public:
    FirstPersonCameraController(const VulkanDevice& device, uint32_t swapChainImageCount, const uint32_t& currentImageIndex, InputManager& inputManager, const BaseCameraSettings& settings = {});

    void move(float dx, float dy, float dz) override;

};