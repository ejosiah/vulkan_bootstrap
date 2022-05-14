#include "VulkanBaseApp.h"

struct Material{
    Texture albedo;
    Texture normal;
    Texture depth;
    Texture distanceMap;
};

struct Object {
    VulkanBuffer vertexBuffer;
    VulkanBuffer indexBuffer;
    uint32_t numIndices;
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

    void render(VkCommandBuffer commandBuffer, Object& object, VkPipeline pipeline, VkPipelineLayout layout, void* constants, uint32_t constantsSize);

    void update(float time) override;

    void checkAppInputs() override;

    void cleanup() override;

    void onPause() override;

    void loadMaterial();

    void createDistanceMap(Texture& depthMap, Texture& distanceMap, bool invert = true);

    void createCube();

    void createPlane();

protected:
    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
        struct {
            float heightScale{0.1};
            int enabled{1};
            int invertDepthMap{0};
        } constants;
    } parallaxOcclusion;

    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
        struct {
            float depth{0.03};
            int enabled{0};
            int invertDepthMap{0};
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

    std::array<Material, 6> materials;
    int currentMaterial = 0;

    static constexpr int MATERIAL_BRICK = 0;
    static constexpr int MATERIAL_BOX = 1;
    static constexpr int MATERIAL_MAN_HOLE = 2;
    static constexpr int MATERIAL_TEXT = 3;
    static constexpr int MATERIAL_NV_EYE = 4;
    static constexpr int MATERIAL_STONE_BRICK = 5;

    VulkanDescriptorSetLayout materialSetLayout;
    VkDescriptorSet materialSet;

    VulkanDescriptorPool descriptorPool;
    VulkanCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    VulkanPipelineCache pipelineCache;

    Object cube;
    Object plane;
    int currentObject = 0;
    static constexpr int OBJECT_CUBE = 0;
    static constexpr int OBJECT_PLANE = 1;
    std::unique_ptr<BaseCameraController> cameraController;

    int strategy = 0;
    static constexpr int OCCLUSION = 0;
    static constexpr int RELIEF_MAPPING = 1;
    static constexpr int DISTANCE_FUNCTION = 2;
};