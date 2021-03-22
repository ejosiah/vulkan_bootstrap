#pragma once

#include "camera_base.h"

static constexpr float DEFAULT_ORBIT_MIN_ZOOM = DEFAULT_ZNEAR;
static constexpr float DEFAULT_ORBIT_MAX_ZOOM = DEFAULT_ZFAR;
static constexpr float DEFAULT_SPEED_ORBIT_ROLL = 100.0f;
static constexpr float DEFAULT_ORBIT_OFFSET_DISTANCE = DEFAULT_ORBIT_MIN_ZOOM + (DEFAULT_ORBIT_MAX_ZOOM - DEFAULT_ORBIT_MIN_ZOOM) * 0.25f;


struct OrbitingCameraSettings : public BaseCameraSettings{
    float offsetDistance = DEFAULT_ORBIT_OFFSET_DISTANCE;
    float orbitRollSpeed = DEFAULT_SPEED_ORBIT_ROLL;
    float orbitMinZoom = DEFAULT_ORBIT_MIN_ZOOM;
    float orbitMaxZoom = DEFAULT_ORBIT_MAX_ZOOM;
    float modelHeight = 1.0f;
    bool preferTargetYAxisOrbiting = true;
};

class OrbitingCameraController : public BaseCameraController {
public:
    OrbitingCameraController(const VulkanDevice& device, uint32_t swapChainImageCount, const uint32_t& currentImageIndex,  InputManager& inputManager, const OrbitingCameraSettings& settings = {});

    void update(float elapsedTime) override;

    void updateModel();

    void move(float dx, float dy, float dz) override;

    void move(const glm::vec3 &direction, const glm::vec3 &amount) override;

    void rotate(float headingDegrees, float pitchDegrees, float rollDegrees) override;

    void undoRoll() override;

    void zoom(float zoom, float minZoom, float maxZoom) override;

    void updateViewMatrix() override;

private:
    float offsetDistance;
    float orbitRollSpeed;
    bool preferTargetYAxisOrbiting;

};