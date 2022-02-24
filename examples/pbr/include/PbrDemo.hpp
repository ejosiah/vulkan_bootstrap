#include "VulkanBaseApp.h"

class PbrDemo : public VulkanBaseApp{
public:
    explicit PbrDemo(const Settings& settings = {});

protected:
    void initApp() override;

    void initCamera();

    void createDescriptorSetLayouts();

    void updateDescriptorSets();

    void initModel();

    void loadEnvironmentMap();

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

    struct {
        VulkanBuffer vertices;
    } screenQuad;

    struct {
        VkDescriptorSet descriptorSet;
        Texture texture;
    } environmentMap;

    VulkanDescriptorSetLayout environmentMapSetLayout;

    VulkanDescriptorPool descriptorPool;
    VulkanCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    VulkanPipelineCache pipelineCache;
    std::unique_ptr<OrbitingCameraController> cameraController;
};