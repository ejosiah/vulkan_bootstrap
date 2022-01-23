#include "VulkanBaseApp.h"

class BloomDemo : public VulkanBaseApp{
public:
    explicit BloomDemo(const Settings& settings = {});

protected:
    void initApp() override;

    void initSceneFrameBuffer();

    void initBlur();

    void initComputeImage(Texture& texture, uint32_t width, uint32_t height);

    void initPostProcessing();

    void blurImage(VkCommandBuffer commandBuffer);

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

    void createGlobalSampler();

    void initQuad();

    void renderUI(VkCommandBuffer commandBuffer);

    void renderOffScreenFramebuffer(VkCommandBuffer commandBuffer);

    void renderScene(VkCommandBuffer commandBuffer);

    void applyPostProcessing(VkCommandBuffer commandBuffer);

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
        alignas(16) glm::vec3 position{0};
        alignas(16) glm::vec3 color{0};
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
        DepthBuffer depthAttachment;
        FramebufferAttachment intensityAttachment;
        VulkanFramebuffer framebuffer;
        VulkanRenderPass renderPass;
        VulkanSampler sampler;
        VulkanPipeline pipeline;
        VulkanPipelineLayout layout;
    } scene;

    struct {
        VulkanPipeline pipeline;
        VulkanPipelineLayout layout;
        VkDescriptorSet intensitySet;
        VkDescriptorSet inSet;
        VkDescriptorSet outSet;
        VulkanDescriptorSetLayout imageSetLayout;
        Texture texture;
        struct {
            std::array<float, 3> offsets{0.0, 1.3846153846, 3.2307692308};
            std::array<float, 3> weights{0.2270270270, 0.3162162162, 0.0702702703};
            int horizontal{1};
        } constants;
    } blur;

    struct {
        VulkanPipeline pipeline;
        VulkanPipelineLayout layout;
        VkDescriptorSet descriptorSet;
        VulkanDescriptorSetLayout setLayout;
        Texture texture;
        struct{
            int gammaOn{1};
            int hdrOn{1};
            int bloomOn{1};
            float exposure{1};
        } constants;
    } postProcess;

    VulkanSampler globalSampler;
    VulkanSampler linearSampler;

    struct {
        VulkanPipeline pipeline;
        VulkanPipelineLayout layout;
        VulkanBuffer vertices;
        VkDescriptorSet descriptorSet;
    } quad;
};