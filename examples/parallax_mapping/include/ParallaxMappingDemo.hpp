#include "VulkanBaseApp.h"

struct Material{
    Texture albedo;
    Texture normal;
    Texture depth;
    Texture distanceMap;
};

class ParallaxMappingDemo : public VulkanBaseApp{
public:
    explicit ParallaxMappingDemo(const Settings& settings = {});

protected:
    void initApp() override;

    void initCamera();

    void createDescriptorPool();

    void createDescriptorSetLayout();

    void updateDescriptorSets(const Material& material);

    void createCommandPool();

    void createPipelineCache();

    void createRenderPipeline();

    void createComputePipeline();

    void onSwapChainDispose() override;

    void onSwapChainRecreation() override;

    void renderUI(VkCommandBuffer commandBuffer);

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override;

    void render(VkCommandBuffer commandBuffer, VkPipeline pipeline, VkPipelineLayout layout, void* constants, uint32_t constantsSize);

    void update(float time) override;

    void checkAppInputs() override;

    void cleanup() override;

    void onPause() override;

    void loadMaterial();

    void createDistanceMap(Texture& depthMap, Texture& distanceMap);

    void createCube();

protected:
    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
        struct {
            float heightScale{0.1};
            int enabled{1};
        } constants;
    } parallaxOcclusion;

    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
        struct {
            float depth{0.03};
            int enabled{0};
        } constants;
    } reliefMapping;

    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
        struct {
            glm::vec3 normalizationFactor{0};
            int numIterations{16};
            int enabled{0};
        } constants;
        int depth{16};
    } distanceMapping;

    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } compute;

    Material bricks;
    Material box;
    int currentMaterial = 0;

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

    int strategy = 0;
    static constexpr int OCCLUSION = 0;
    static constexpr int RELIEF_MAPPING = 1;
    static constexpr int DISTANCE_FUNCTION = 2;
};