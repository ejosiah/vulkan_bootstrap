#include "VulkanBaseApp.h"

class BloomDemo : public VulkanBaseApp{
public:
    explicit BloomDemo(const Settings& settings = {});

protected:
    void initApp() override;

    void initBloomFramebuffer();

    void createDescriptorPool();

    void createCommandPool();

    void createDescriptorSetLayouts();

    void updateDescriptorSets();

    void createPipelineCache();

    void createRenderPipeline();

    void createSceneObjects();

    void onSwapChainDispose() override;

    void onSwapChainRecreation() override;

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override;

    void update(float time) override;

    void checkAppInputs() override;

    void cleanup() override;

    void onPause() override;

    void initCamera();

    void initLights();

    void initTextures();

    void initQuad();

    void renderUI(VkCommandBuffer commandBuffer);

    void renderOffScreenFramebuffer(VkCommandBuffer commandBuffer);

    void renderScene(VkCommandBuffer commandBuffer);

    void applyBloom(VkCommandBuffer commandBuffer);

protected:
    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } lightRender;

    VulkanDescriptorSetLayout textureLayoutSet;
    VulkanDescriptorSetLayout lightLayoutSet;

    struct {
        VkDescriptorSet wood;
        VkDescriptorSet container;
        VkDescriptorSet light;
    } descriptorSets;

    struct {
        Texture wood;
        Texture container;
    } textures;

    struct Light{
        alignas(16) glm::vec3 position;
        alignas(16) glm::vec3 color;
    };

    struct {
        VulkanBuffer vertices;
        VulkanBuffer indices;
        std::vector<glm::mat4> instanceTransforms;
    } cube;

    VulkanBuffer lightBuffer;

    std::vector<Light> lights;

    VulkanDescriptorPool descriptorPool;
    VulkanCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    VulkanPipelineCache pipelineCache;
    std::unique_ptr<SpectatorCameraController> cameraController;

    struct {
        ColorBuffer colorAttachment;
        ColorBuffer compositeAttachment;
        DepthBuffer depthAttachment;
        FramebufferAttachment brightnessAttachment;
        std::array<FramebufferAttachment, 2> blurAttachment;
        VulkanFramebuffer framebuffer;
        VulkanRenderPass renderPass;
        VulkanSampler sampler;
    } scene;


    struct{
        struct {
            VulkanPipeline pipeline;
            VulkanPipelineLayout layout;
            uint32_t subpass = 0;
        } scene;

        struct {
            VulkanPipeline pipeline;
            VulkanPipelineLayout layout;
            uint32_t subpass = 1;
        } blur;
        struct {
            VulkanPipeline pipeline;
            VulkanPipelineLayout layout;
            VkDescriptorSet colorAttachmentsSet;
            uint32_t subpass = 2;
        } composite;
    } subpasses;

    struct {
        int gammaOn{1};
        int hdrOn{1};
        int bloomOn{1};
        float exposure{1};
    } compositeConstants;

    VulkanDescriptorSetLayout compositeSetLayout;

    VulkanDescriptorSetLayout blurInLayout;
    std::array<VkDescriptorSet, 3> blurInSet;

    Texture testTexture;

    struct {
        VulkanPipeline pipeline;
        VulkanPipelineLayout layout;
        VulkanBuffer vertices;
        VkDescriptorSet descriptorSet;
    } quad;
};