#pragma once

#include "VulkanBaseApp.h"

class DeferredRendering : public VulkanBaseApp{
public:
    explicit DeferredRendering(const Settings& settings = {});

protected:
    void initApp() override;

    void swapChainReady() override;

    void createFramebuffer() override;

    void initCamera();

    void loadSponza();

    void initLights();

    void createScreenBuffer();

    void setupInput();

    void createDescriptorPool();

    void createDescriptorSetLayouts();

    void createGBuffer();

    void updateDescriptorSets();

    void createCommandPool();

    void createPipelineCache();

    void createPipelines();

    void createRenderPipeline(GraphicsPipelineBuilder& builder);

    void createDepthPrePassPipeline(GraphicsPipelineBuilder& builder);

    void createGBufferPipeline(GraphicsPipelineBuilder& builder);

    void onSwapChainDispose() override;

    void onSwapChainRecreation() override;

    RenderPassInfo buildRenderPass() override;

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override;

    void update(float time) override;

    void checkAppInputs() override;

    void cleanup() override;

    void onPause() override;

protected:
    VulkanDescriptorPool descriptorPool;
    VulkanCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    VulkanPipelineCache pipelineCache;
    VulkanDrawable sponza;
    std::unique_ptr<CameraController> cameraController;

    struct CameraProps{
        float near{0};
        float far{0};
        int isLight{0};
    } cameraProps;

    struct GBuffer {
        std::vector<FramebufferAttachment> albedo;
        std::vector<FramebufferAttachment> normal;
        std::vector<FramebufferAttachment> position;
        std::vector<FramebufferAttachment> emission;
        VulkanDescriptorSetLayout setLayout;
        std::vector<VkDescriptorSet> descriptorSets;
    } gBuffer;

    struct Pipelines{
        struct {
            VulkanPipelineLayout layout;
            VulkanPipeline pipeline;
        } depthPrePass;

        struct {
            VulkanPipelineLayout layout;
            VulkanPipeline pipeline;
        } gBuffer;

        struct {
            VulkanPipelineLayout layout;
            VulkanPipeline pipeline;
        } render;
    } pipelines;

    VulkanBuffer screenBuffer;

    struct {
        uint32_t numLights;
        uint32_t displayOption{DISPLAY_LIGHTING};
    } renderConstants{};

    struct Light{
        glm::vec4 position;
        glm::vec4 color;
    };

    struct {
        std::vector<VulkanBuffer> vertices;
        std::vector<VulkanBuffer> indices;
        VulkanBuffer positions;
        uint32_t count{50};
        VulkanDescriptorSetLayout setLayout;
        VkDescriptorSet descriptorSet;
    } lights;

    static const std::string kAttachment_GBUFFER_ALBDEO;
    static const std::string kAttachment_GBUFFER_NORMAL;
    static const std::string kAttachment_GBUFFER_POSITION;
    static const std::string kAttachment_GBUFFER_EMISSION;

   static constexpr uint32_t kSubpass_DEPTH = 0;
   static constexpr uint32_t kSubpass_GBUFFER = 1;
   static constexpr uint32_t kSubpass_RENDER = 2;

   static constexpr uint32_t kLayoutBinding_ALBDO = 0;
   static constexpr uint32_t kLayoutBinding_NORMAL = 1;
   static constexpr uint32_t kLayoutBinding_POSITION = 2;
   static constexpr uint32_t kLayoutBinding_EMISSION = 3;

   enum DisplayOption{
       DISPLAY_LIGHTING,
       DISPLAY_ALBEDO,
       DISPLAY_NORMAL,
       DISPLAY_POSITION,
       DISPLAY_DEPTH,
       MAX_DISPLAY_OPTIONS
   };

   Action* selectOption;
};