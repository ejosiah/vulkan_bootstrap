#pragma once

#include "camera_base.h"
#include <limits>

static constexpr float DEFAULT_ORBIT_MIN_ZOOM = DEFAULT_ZOOM_MIN;
static constexpr float DEFAULT_ORBIT_MAX_ZOOM = DEFAULT_ZOOM_MAX;
static constexpr float DEFAULT_SPEED_ORBIT_ROLL = 100.0f;
static constexpr float DEFAULT_ORBIT_OFFSET_DISTANCE = DEFAULT_ORBIT_MIN_ZOOM + (DEFAULT_ORBIT_MAX_ZOOM - DEFAULT_ORBIT_MIN_ZOOM) * 0.25f;


struct OrbitingCameraSettings : public BaseCameraSettings{
    float offsetDistance = DEFAULT_ORBIT_OFFSET_DISTANCE;
    float orbitRollSpeed = DEFAULT_SPEED_ORBIT_ROLL;
    float orbitMinZoom = DEFAULT_ORBIT_MIN_ZOOM;
    float orbitMaxZoom = DEFAULT_ORBIT_MAX_ZOOM;
    float modelHeight = 1.0f;
    bool preferTargetYAxisOrbiting = true;
    glm::vec3 target{std::numeric_limits<float>::quiet_NaN()};
    struct {
        glm::vec3 min{MAX_FLOAT};
        glm::vec3 max{MIN_FLOAT};
    } model;
};

// FIXME change to focus on a target and not a model in the scene
class OrbitingCameraController : public BaseCameraController {
public:
    OrbitingCameraController(const VulkanDevice& device, uint32_t swapChainImageCount, const uint32_t& currentImageIndex,  InputManager& inputManager, const OrbitingCameraSettings& settings = {});

    void update(float elapsedTime) override;

    void updateModel(const glm::vec3& position, const glm::quat& orientation = {1, 0, 0, 0});

    void move(float dx, float dy, float dz) override;

    void move(const glm::vec3 &direction, const glm::vec3 &amount) override;

    void rotate(float headingDegrees, float pitchDegrees, float rollDegrees) override;

    void undoRoll() override;

    void zoom(float zoom, float minZoom, float maxZoom) override;

    void updateViewMatrix() override;

    void onPositionChanged() final;

    void push(VkCommandBuffer commandBuffer, VkPipelineLayout layout, VkShaderStageFlags stageFlags = VK_SHADER_STAGE_VERTEX_BIT) const override;

    void push(VkCommandBuffer commandBuffer, VkPipelineLayout layout, const glm::mat4 &model, VkShaderStageFlags stageFlags = VK_SHADER_STAGE_VERTEX_BIT) override;

private:
    float offsetDistance;
    float orbitRollSpeed;
    bool preferTargetYAxisOrbiting;

    mutable struct {
        glm::vec3 position;
        glm::quat orientation;
    } model;

};