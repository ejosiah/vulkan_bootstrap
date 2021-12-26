#include "VulkanBaseApp.h"
#include "ImGuiPlugin.hpp"

struct RotationAxis{};
struct XAxis{};
struct YAxis{};
struct ZAxis{};

enum RotationOrder{
    XYZ, XZY, YXZ, YZX, ZXY, ZYX
};

class RotationDemo : public VulkanBaseApp{
public:
    explicit RotationDemo(const Settings& settings = {});

protected:
    void initApp() override;

    void createCamera();

    void createRotationAxis();

    void loadModel();

    void updateModelParentTransform();

    Entity createAxis(const std::string& name, float innerR, float outerR, glm::vec3 color, glm::quat orientation);

    void createDescriptorPool();

    void createCommandPool();

    void createPipelineCache();

    void createRenderPipeline();

    void createComputePipeline();

    void onSwapChainDispose() override;

    void onSwapChainRecreation() override;

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override;

    void update(float time) override;

    void checkAppInputs() override;

    void cleanup() override;

    void onPause() override;

    void renderUI(VkCommandBuffer commandBuffer);

protected:
    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } render;

    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } compute;

    VulkanDescriptorPool descriptorPool;
    VulkanCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    VulkanPipelineCache pipelineCache;
    std::unique_ptr<OrbitingCameraController> cameraController;
    glm::vec3 axisRotation{0};
    int rotationOrder = XYZ;
    bool orderUpdated = false;
    bool moveCamera = false;
    Entity spaceShip;
};