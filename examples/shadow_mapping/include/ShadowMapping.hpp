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
    // Depth bias (and slope) are used to avoid shadowing artifacts
    // Constant depth bias factor (always applied)
    float depthBiasConstant{1.25f};
    // Slope depth bias factor, applied depending on polygon's slope
    float depthBiasSlope{1.75f};
};

enum class LightType : int {
    DIRECTIONAL = 0,
    POSITIONAL = 1
};

class ShadowMapping : public VulkanBaseApp{
public:
    explicit ShadowMapping(const Settings& settings = {});

protected:
    void initApp() override;

    void initCamera();

    void initShadowMap();

    void initFrustum();


    void createUboBuffer();

    void updateUbo();

    void createFloor();

    void createDescriptorPool();

    void createDescriptorSetLayouts();

    void updateDescriptorSets();

    void updateUboDescriptorSet();

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

    void displayUI(VkCommandBuffer commandBuffer);

    void update(float time) override;

    void checkAppInputs() override;

    void cleanup() override;

    void onPause() override;

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

    struct {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 projection;
        glm::mat4 lightSpaceMatrix;
    } ubo;

    VulkanBuffer uboBuffer;

    VulkanDescriptorPool descriptorPool;
    VulkanCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    VulkanPipelineCache pipelineCache;
    glm::vec4 lightDir{0, 15, 5, 1};
    Object cubes;
    std::unique_ptr<CameraController> cameraController;
    ShadowMap shadowMap;
    bool displayFrustum{true};
    bool shadowMapInvalidated{false};
    LightType lightType{LightType::DIRECTIONAL};
};