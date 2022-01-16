#include "VulkanBaseApp.h"


class HighDynamicRangeDemo : public VulkanBaseApp{
public:
    explicit HighDynamicRangeDemo(const Settings& settings = {});

protected:
    void initApp() override;

    void initCamera();

    void initLights();

    void createTunnel();

    void loadTexture();

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

    void renderUI(VkCommandBuffer commandBuffer);

    void update(float time) override;

    void checkAppInputs() override;

    void cleanup() override;

    void onPause() override;

protected:
    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } render;

    VulkanDescriptorSetLayout setLayout;
    VkDescriptorSet descriptorSet;

    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } compute;

    VulkanDescriptorPool descriptorPool;
    VulkanCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    VulkanPipelineCache pipelineCache;
    std::unique_ptr<SpectatorCameraController> cameraController;

    struct{
        int gammaCorrect{0};
        int hdrEnabled{0};
        float exposure{1};
    } displaySettings;

    struct Light{
        alignas(16) glm::vec3 position;
        alignas(16) glm::vec3 color;
    };
    VulkanBuffer lightBuffer;

    std::vector<Light> lights;
    Texture woodTexture;

    struct {
        VulkanBuffer vertices;
        VulkanBuffer indices;
        glm::mat4 model;
    } tunnel;
};