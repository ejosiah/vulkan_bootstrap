#include "VulkanBaseApp.h"
#include "ImGuiPlugin.hpp"
#include "scene.hpp"

class RotationDemo : public VulkanBaseApp{
public:
    explicit RotationDemo(const Settings& settings = {});

protected:
    void initApp() override;

    void createCamera();

    void createScenes();

    void createDescriptorPool();

    void createCommandPool();

    void createPipelineCache();

    void onSwapChainDispose() override;

    void onSwapChainRecreation() override;

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override;

    void update(float time) override;

    void checkAppInputs() override;

    void cleanup() override;

    void onPause() override;

    void renderUI(VkCommandBuffer commandBuffer);

protected:

    VulkanDescriptorPool descriptorPool;
    VulkanCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    VulkanPipelineCache pipelineCache;
    std::unique_ptr<OrbitingCameraController> cameraController;
    bool moveCamera = false;
    int currentScene = 1;
    std::map<int, std::unique_ptr<Scene>> scenes;
};