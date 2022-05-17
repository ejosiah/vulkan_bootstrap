#include "VulkanBaseApp.h"

struct Patch{
    VulkanBuffer vertices;
};

struct Pipeline{
    VulkanPipelineLayout layout;
    VulkanPipeline pipeline;
};

class TessellationDemo : public VulkanBaseApp{
public:
    explicit TessellationDemo(const Settings& settings = {});

protected:
    void initApp() override;

    void initCamera();

    void createDescriptorPool();

    void createCommandPool();

    void createPipelineCache();

    void createRenderPipeline();

    void createComputePipeline();

    void createPatches();

    void onSwapChainDispose() override;

    void onSwapChainRecreation() override;

    void renderUI(VkCommandBuffer commandBuffer);

    void renderPatch(VkCommandBuffer commandBuffer);

    void render(VkCommandBuffer commandBuffer, Pipeline& pipeline, Patch& patch, const void* constants, uint32_t constantsSize);

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override;

    void update(float time) override;

    void checkAppInputs() override;

    void cleanup() override;

    void onPause() override;

protected:
    struct {
        Pipeline equalSpacing;
        Pipeline fractionalEvenSpacing;
        Pipeline fractionalOddSpacing;
        struct {
            glm::ivec4 outerTessLevels{4};
            glm::ivec2 innerTessLevels{4};
        } constants;
    } quad;

    struct {
        Pipeline equalSpacing;
        Pipeline fractionalEvenSpacing;
        Pipeline fractionalOddSpacing;
        struct {
            glm::ivec2 outerTessLevels{4};
        } constants;
    } isoline;

    struct {
        Pipeline equalSpacing;
        Pipeline fractionalEvenSpacing;
        Pipeline fractionalOddSpacing;
        struct {
            glm::ivec3 outerTessLevels{3};
            int innerTessLevel{1};
        } constants;
    } tri;

    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } compute;

    VulkanDescriptorPool descriptorPool;
    VulkanCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    VulkanPipelineCache pipelineCache;
    Patch quadPatch;
    Patch trianglePatch;
    Patch isolinePatch;
    Camera camera;

    int patchType = 0;
    static constexpr int PATCH_TYPE_QUADS = 0;
    static constexpr int PATCH_TYPE_TRIANGLE = 1;
    static constexpr int PATCH_TYPE_ISOLINES = 2;

    int spacing = 0;
    static constexpr int SPACING_EQUAL = 0;
    static constexpr int SPACING_FRACTIONAL_EVEN = 1;
    static constexpr int SPACING_FRACTIONAL_ODD = 2;
};