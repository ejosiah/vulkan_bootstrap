#include "VulkanBaseApp.h"

class ParallaxMappingDemo : public VulkanBaseApp{
public:
    explicit ParallaxMappingDemo(const Settings& settings = {});

protected:
    void initApp() override;

    void initCamera();

    void createDescriptorPool();

    void createDescriptorSetLayout();

    void updateDescriptorSets();

    void createCommandPool();

    void createPipelineCache();

    void createRenderPipeline();

    void createComputePipeline();

    void onSwapChainDispose() override;

    void onSwapChainRecreation() override;

    void renderUI(VkCommandBuffer commandBuffer);

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override;

    void update(float time) override;

    void checkAppInputs() override;

    void cleanup() override;

    void onPause() override;

    void loadMaterial();

    void createCube();

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
        Texture albedo;
        Texture normal;
        Texture depth;
    } material;

    VulkanDescriptorSetLayout materialSetLayout;
    VkDescriptorSet materialSet;

    VulkanDescriptorPool descriptorPool;
    VulkanCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    VulkanPipelineCache pipelineCache;

    struct {
        VulkanBuffer vertexBuffer;
        VulkanBuffer indexBuffer;
        uint32_t numIndices;
    } cube;
    std::unique_ptr<BaseCameraController> cameraController;
    float heightScale{0.1};
};