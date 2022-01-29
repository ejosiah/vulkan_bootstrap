#pragma once

#include "VulkanBaseApp.h"
#include <vhacd/VHACD.h>
#include "ThreadPool.hpp"
#include "oclHelper.h"
#include "convexHullbuilder.hpp"

struct Model{
    VulkanDrawable drawable;
    glm::mat4 transform{1};
    std::atomic_bool ready{false};
    int pipeline{0};
};

class ConvexDecompositionDemo : public VulkanBaseApp{
public:
    explicit ConvexDecompositionDemo(const Settings& settings = {});

protected:
    void initApp() override;

    void initCHBuilder();

    void createDescriptorPool();

    void loadBunny();

    void loadDragon();

    void loadArmadillo();

    void loadSkull();

    void loadWereWolf();

    std::future<par::done> loadModel(const std::string& path, int index);

    std::future<par::done> loadModel(const std::string& path, int index, std::function<void(Model&)>&& postLoadOp);

    void currentModelChanged();

    void updateHACDData();

    void initCamera();

    glm::vec3 getCameraTarget(Model& model);

    void createCommandPool();

    void createPipelineCache();

    void createRenderPipeline();

    void createFloor();

    void onSwapChainDispose() override;

    void onSwapChainRecreation() override;

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override;

    void renderModel(VkCommandBufferInheritanceInfo& inheritanceInfo);

    void renderFloor(VkCommandBufferInheritanceInfo& inheritanceInfo);

    void mirror(VkCommandBufferInheritanceInfo& inheritanceInfo);

    glm::mat4 positionModel(Model& model, int multiplier = 1);

    std::tuple<glm::vec3, glm::vec3> getBounds(Model& model);

    void update(float time) override;

    void checkAppInputs() override;

    void cleanup() override;

    void onPause() override;

    void renderUI(VkCommandBufferInheritanceInfo& inheritanceInfo);

    void renderConvexHull(VkCommandBufferInheritanceInfo& inheritanceInfo);

    bool shouldUpdateConvexHull();

    void constructConvexHull();

    VHACD::IVHACD::Parameters getParams();

protected:
    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } render[2];

    struct{
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } mirrorRender[2];

    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } ch;

    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
        struct{
            Camera camera{};
            glm::vec4 color{1, 1, 0, 1};
            float length{0.1};
        } constants;
    } normals;

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

    int currentModel = 0;
    int nextModel = 0;
    std::array<Model, 5> models;

    std::array<ConvexHulls, 2> convexHulls;
    std::array<std::atomic_bool, 2> hullIsReady{false, false};
    std::atomic_int currentHull = 1;

    VulkanDescriptorPool descriptorPool;
    VulkanCommandPool commandPool;
    VulkanCommandPool secondaryCommandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    VulkanPipelineCache pipelineCache;
    std::unique_ptr<OrbitingCameraController> cameraController;
    std::array<std::vector<VkCommandBuffer>, MAX_IN_FLIGHT_FRAMES> secondaryCommandBuffers;
    ConvexHullBuilder convexHullBuilder;

    enum Commands : uint32_t {
        RENDER_MODEL, MIRROR_SCENE, RENDER_FLOOR, RENDER_CONVEX_HULL, RENDER_UI, NUM_COMMANDS
    };

    Callback callbackVHACD;

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
        int maxNumVerticesPerCH{4};
        float minVolumePerCH{0.0001};
        int convexHullApproximation{1};
        int oclAcceleration{1};
        int oclPlatformID{0};
        int oclDeviceID{0};
        bool projectHullVertices{true};
    } params;
    bool updateHulls{false};
    float easeInDuration{2.0};

    std::atomic_bool startEasing{false};
};