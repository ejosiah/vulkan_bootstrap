#include "VulkanBaseApp.h"


class DeferredRendering : public VulkanBaseApp{
public:
    explicit DeferredRendering(const Settings& settings = {});

protected:
    void initApp() override;

    void initCamera();

    void loadSponza();

    void createScreenBuffer();

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
        float near;
        float far;
    } cameraProps;

    struct GBuffer {
        FramebufferAttachment albedo;
        FramebufferAttachment normal;
        FramebufferAttachment depth;
        VulkanDescriptorSetLayout setLayout;
        VkDescriptorSet descriptorSet;
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

//   static constexpr uint32_t kAttachment_BACK = 0;
//   static constexpr uint32_t kAttachment_DEPTH = 1;
//   static constexpr uint32_t kAttachment_GBUFFER = 2;
    static const std::string kAttachment_GBUFFER_ALBDEO;
    static const std::string kAttachment_GBUFFER_NORMAL;
    static const std::string kAttachment_GBUFFER_DEPTH;

   static constexpr uint32_t kSubpass_DEPTH = 0;
   static constexpr uint32_t kSubpass_GBUFFER = 1;
   static constexpr uint32_t kSubpass_RENDER = 1;

   static constexpr uint32_t kLayoutBinding_ALBDO = 0;
   static constexpr uint32_t kLayoutBinding_NORMAL = 1;
   static constexpr uint32_t kLayoutBinding_DEPTH = 2;
};