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

    void updateSsaoDescriptorSet();

    void createCommandPool();

    void createPipelineCache();

    void createRenderPipeline();

    void createComputePipeline();

    void onSwapChainDispose() override;

    void onSwapChainRecreation() override;

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override;

    void renderUI(VkCommandBuffer commandBuffer);

    void renderScene(VkCommandBuffer commandBuffer);

    void ssaoPass(VkCommandBuffer commandBuffer);

    void blurPass(VkCommandBuffer commandBuffer);

    void update(float time) override;

    void checkAppInputs() override;

    void cleanup() override;

    void onPause() override;

    void createSsaoSamplingData();

    void createSsaoFrameBuffer();

    void createBlurFrameBuffer();

    VulkanDrawable model;

protected:
    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } render;

    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
        struct{
            glm::mat4 view;
            int aoOnly{1};
            int aoOn{1};
        } constants;
    } lighting;

    struct {
        VulkanBuffer vertices;
        uint32_t numVertices{0};
    } quad;

    struct {
        DepthBuffer depth;
        FramebufferAttachment position;
        FramebufferAttachment normal;
        FramebufferAttachment color;
        VulkanRenderPass renderpass;
        VulkanFramebuffer framebuffer;
    } gBuffer;

    struct{
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
        FramebufferAttachment occlusion;
        VulkanRenderPass renderpass;
        VulkanFramebuffer framebuffer;
        struct{
            VulkanPipelineLayout layout;
            VulkanPipeline pipeline;
            FramebufferAttachment blurOutput;
            VulkanRenderPass renderpass;
            VulkanFramebuffer framebuffer;
            int on{1};
            VkDescriptorSet descriptorSet;
        } blur;
        VulkanDescriptorSetLayout setLayout;
        VkDescriptorSet descriptorSet;
        VulkanBuffer samples;
        Texture noise;
        struct{
            glm::mat4 projection{1};
            int kernelSize{32};
            float radius{0.3};
            float bias{0.025};
        } constants;
    } ssao;

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