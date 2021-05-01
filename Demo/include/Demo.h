#pragma once

#include "VulkanBaseApp.h"
#include "VulkanModel.h"
#include "Phong.h"

constexpr float     FLOOR_WIDTH = 8.0f;
constexpr float     FLOOR_HEIGHT = 8.0f;
constexpr float     FLOOR_TILE_S = 8.0f;
constexpr float     FLOOR_TILE_T = 8.0f;

constexpr float     CAMERA_FOVX = 90;
constexpr float     CAMERA_ZFAR = 100.0f;
constexpr float     CAMERA_ZNEAR = 0.1f;
//constexpr float     CAMERA_ZOOM_MAX = 2.0f;
//constexpr float     CAMERA_ZOOM_MIN = 1.0f;
constexpr float     CAMERA_ZOOM_MAX = 5.0f;
constexpr float     CAMERA_ZOOM_MIN = 1.5f;

constexpr float     CAMERA_SPEED_FLIGHT_YAW = 100.0f;
constexpr float     CAMERA_SPEED_ORBIT_ROLL = 100.0f;

constexpr glm::vec3   CAMERA_ACCELERATION(4.0f, 4.0f, 4.0f);
constexpr glm::vec3   CAMERA_VELOCITY(1.0f, 1.0f, 1.0f);

struct Material{
    alignas(16) glm::vec3 ambient;
    alignas(16) glm::vec3 diffuse;
    alignas(16) glm::vec3 specular;
    float shininess;
};

struct Floor{
    struct {
        float width = 8.0f;
        float height = 8.0f;
    } dimensions{};

    VulkanBuffer vertices;
    VulkanBuffer indices;
    uint32_t indexCount;
    Texture texture;
    Texture lightMap;
    VulkanDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
};

class Demo : public VulkanBaseApp{
public:
    explicit Demo(const Settings& settings);

protected:
    void initApp() override;

    void initCamera();

    void createDescriptorPool();

    void loadSpaceShip();

    void loadFloor();

    void createFloorDescriptors();

    void createPipelines();

    void onSwapChainDispose() override;

    void onSwapChainRecreation() override;

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override;

    void update(float time) override;

    void checkAppInputs() override;

    void cleanup() override;

    std::string menu() const;

    void displayMenu(VkCommandBuffer commandBuffer);

private:
    VulkanCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    VulkanDescriptorPool descriptorPool;

    VulkanDrawable spaceShip;
    Floor floor;
    std::unique_ptr<CameraController> cameraController;
    struct {
        VulkanPipeline spaceShip;
        VulkanPipeline floor;
    } pipelines;

    struct{
        VulkanPipelineLayout spaceShipLayout;
        VulkanPipelineLayout floorLayout;
    } layouts;
    bool displayHelp = false;

    struct {
        Action* help;
        Action* toggleVSync;
        Action* fullscreen;
    } actions;

    struct {
        Action* firstPerson;
        Action* spectator;
        Action* flight;
        Action* orbit;
    } cameraModes;

    int msaaSamples = 1;
    int maxAnisotrophy = 1;
    std::string stuff;
};