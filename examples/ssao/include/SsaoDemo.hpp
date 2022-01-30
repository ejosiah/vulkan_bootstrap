#include "VulkanBaseApp.h"

class SsaoDemo : public VulkanBaseApp{
public:
    explicit SsaoDemo(const Settings& settings = {});

protected:
    void initApp() override;

    void loadModel();

    void initCamera();

    void initGBuffer();

    void initScreenQuad();

    void createGlobalSampler();

    void createDescriptorPool();

    void createDescriptorSetLayout();

    void updateDescriptorSet();

    void createCommandPool();

    void createPipelineCache();

    void createRenderPipeline();

    void createComputePipeline();

    void onSwapChainDispose() override;

    void onSwapChainRecreation() override;

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override;

    void renderScene(VkCommandBuffer commandBuffer);

    void update(float time) override;

    void checkAppInputs() override;

    void cleanup() override;

    void onPause() override;

    VulkanDrawable model;

protected:
    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } render;

    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
        VkDescriptorSet descriptorSet;
        VulkanBuffer vertices;
        uint32_t numVertices{0};
    } quad;

    struct {
        DepthBuffer depth;
        ColorBuffer position;
        ColorBuffer normal;
        VulkanRenderPass renderpass;
        VulkanFramebuffer framebuffer;
    } gBuffer;

    VulkanDescriptorSetLayout textureSetLayout;
    VulkanSampler globalSampler;

    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } compute;

    VulkanDescriptorPool descriptorPool;
    VulkanCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    VulkanPipelineCache pipelineCache;
    std::unique_ptr<SpectatorCameraController> cameraController;
};