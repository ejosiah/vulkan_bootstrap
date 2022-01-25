#include "VulkanBaseApp.h"
#include <vhacd/VHACD.h>

struct ConvexHulls{
    std::vector<VulkanBuffer> vertices;
    std::vector<VulkanBuffer> indices;
    std::vector<glm::vec4> colors;
    std::vector<float> points;
    std::vector<uint32_t> triangles;
};

class ConvextDecompositionDemo : public VulkanBaseApp{
public:
    explicit ConvextDecompositionDemo(const Settings& settings = {});

protected:
    void initApp() override;

    void createDescriptorPool();

    void loadBunny();

    void updateHACDData();

    void initCamera();

    void createCommandPool();

    void createPipelineCache();

    void createRenderPipeline();

    void createFloor();

    void onSwapChainDispose() override;

    void onSwapChainRecreation() override;

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override;

    VkCommandBuffer renderScene(VkCommandBufferInheritanceInfo& inheritanceInfo);

    VkCommandBuffer renderFloor(VkCommandBufferInheritanceInfo& inheritanceInfo);

    VkCommandBuffer mirror(VkCommandBufferInheritanceInfo& inheritanceInfo);

    void update(float time) override;

    void checkAppInputs() override;

    void cleanup() override;

    void onPause() override;

    VkCommandBuffer renderUI(VkCommandBufferInheritanceInfo& inheritanceInfo);

    VkCommandBuffer renderConvexHull(VkCommandBufferInheritanceInfo& inheritanceInfo);

    void initVHACD();

    bool shouldUpdateConvexHull();

    void constructConvexHull();

    VHACD::IVHACD::Parameters getParams();

protected:
    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } render;

    struct{
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } mirrorRender;

    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } ch;

    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } mirrorCH;

    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
        VulkanBuffer vertices;
        VulkanBuffer indices;
    } floor;

    VulkanDrawable bunny;
    std::array<ConvexHulls, 2> convexHulls;
    std::array<std::atomic_bool, 2> hullIsReady{false, false};
    int currentHull = 1;
    std::future<bool> modelIsReady;

    VulkanDescriptorPool descriptorPool;
    VulkanCommandPool commandPool;
    VulkanCommandPool secondaryCommandPool;
    VulkanCommandPool commandPoolCH;
    std::vector<VkCommandBuffer> commandBuffers;
    VulkanPipelineCache pipelineCache;
    std::unique_ptr<OrbitingCameraController> cameraController;

    struct {
        int resolution{100000};
        int maxHulls{1024};
        float concavity{0.0025};
        int planeDownSampling{4};
        int convexHullDownSampling{4};
        float alpha{0.05};
        float beta{0.05};
        float gamma{0.00125};
        float delta{0.05};
        int pca{0};
        int mode{0};
        int maxNumVerticesPerCH{64};
        float minVolumePerCH{0.001};
        int convexHullApproximation{1};
        int oclAcceleration{0};
        int oclPlatformID{0};
        int oclDeviceID{0};
        bool projectHullVertices{true};
    } params;
    bool updateHulls{false};

    VHACD::IVHACD* interfaceVHACD;
};