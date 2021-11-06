#include "VulkanBaseApp.h"
#include "BulletPhysicsPlugin.hpp"

constexpr float     CAMERA_FOVX = 90;
constexpr float     CAMERA_ZFAR = 100.0f;
constexpr float     CAMERA_ZNEAR = 0.1f;
constexpr float     CAMERA_ZOOM_MAX = 5.0f;
constexpr float     CAMERA_ZOOM_MIN = 1.5f;
constexpr glm::vec3   CAMERA_ACCELERATION(4.0f, 4.0f, 4.0f);
constexpr glm::vec3   CAMERA_VELOCITY(1.0f, 1.0f, 1.0f);

class BulletPhysicsDemo : public VulkanBaseApp{
public:
    explicit BulletPhysicsDemo(const Settings& settings = {});

protected:
    void initApp() override;

    void initCamera();

    void createDescriptorPool();

    void createCommandPool();

    void createPipelineCache();

    void createRenderPipeline();

    void createComputePipeline();

    void onSwapChainDispose() override;

    void onSwapChainRecreation() override;

    void createRigidBodies();

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override;

    void update(float time) override;

    void checkAppInputs() override;

    void cleanup() override;

    void onPause() override;

    void displayInfo(VkCommandBuffer commandBuffer);

protected:
    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } render;

    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } compute;

    VulkanDescriptorPool descriptorPool;
    VulkanCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    VulkanPipelineCache pipelineCache;
    std::unique_ptr<CameraController> cameraController;
};