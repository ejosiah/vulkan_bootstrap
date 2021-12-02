#include "VulkanBaseApp.h"

struct Object {
    VulkanBuffer vertices;
    VulkanBuffer indices;
    VulkanBuffer transforms;
    uint32_t instanceCount{0};
};

struct ShadowMap{
    VulkanPipeline pipeline;
    VulkanPipelineLayout layout;
    FramebufferAttachment framebufferAttachment;
    VulkanFramebuffer framebuffer;
    VulkanRenderPass renderPass;
    VulkanSampler sampler;
    glm::mat4 lightProjection{1};
    glm::mat4 lightView{1};
    glm::mat4 lightSpaceMatrix{1};
    uint32_t size{2048};
};

class ShadowMapping : public VulkanBaseApp{
public:
    explicit ShadowMapping(const Settings& settings = {});

protected:
    void initApp() override;

    void initCamera();

    void initShadowMap();

    void initFrustum();

    void createFloor();

    void createDescriptorPool();

    void createDescriptorSetLayouts();

    void updateShadowMapDescriptorSet();

    void createCommandPool();

    void createPipelineCache();

    void createRenderPipeline();

    void createComputePipeline();

    void onSwapChainDispose() override;

    void onSwapChainRecreation() override;

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override;

    void constructShadowMap(VkCommandBuffer commandBuffer);

    void drawCubes(VkCommandBuffer commandBuffer);

    void drawFrustum(VkCommandBuffer commandBuffer);

    void update(float time) override;

    void checkAppInputs() override;

    void cleanup() override;

    void onPause() override;

    void waitToCreateShadowMap(VkCommandBuffer commandBuffer);

    void waitForShadowMapToBeReady(VkCommandBuffer commandBuffer);

protected:
    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
        VulkanDescriptorSetLayout uboDescriptorSetLayout;
        VulkanDescriptorSetLayout shadowMapDescriptorSetLayout;
        VkDescriptorSet shadowMapDescriptorSet;
        VkDescriptorSet uboDescriptorSet;
    } render;

    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } compute;

    struct {
        VulkanBuffer vertices;
        VulkanBuffer indices;
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } frustum;

    VulkanDescriptorPool descriptorPool;
    VulkanCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    VulkanPipelineCache pipelineCache;
    glm::vec4 lightDir{0, 15, 5, 1};
    Object cubes;
    std::unique_ptr<CameraController> cameraController;
    VkEvent createShadowMapEvent;
    VkEvent shadowMapReadyEvent;
    ShadowMap shadowMap;
};