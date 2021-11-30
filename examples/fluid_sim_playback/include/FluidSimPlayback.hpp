#include "VulkanBaseApp.h"
#include "Animation.hpp"

class FluidSimPlayback : public VulkanBaseApp{
public:
    explicit FluidSimPlayback(const Settings& settings = {});

protected:
    void initApp() override;

    void initCamera();

    void loadAnimation();

    void createDescriptorPool();

    void createCommandPool();

    void createPipelineCache();

    void createRenderPipeline();

    void createComputePipeline();

    void onSwapChainDispose() override;

    void onSwapChainRecreation() override;

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override;

    void update(float time) override;

    void checkAppInputs() override;

    void cleanup() override;

    void onPause() override;

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
    std::shared_ptr<OrbitingCameraController> camera;
    Animation animation;
    VulkanBuffer vertexBuffer;
    uint32_t numFrames;
    uint32_t numPoints;
    uint32_t fps;
    glm::vec4 pointColor{1, 0, 0, 1};
};