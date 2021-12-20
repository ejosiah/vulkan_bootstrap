#include "VulkanBaseApp.h"
#include "components.h"
#include "SkyBox.hpp"

class EnttDemo : public VulkanBaseApp{
public:
    explicit EnttDemo(const Settings& settings = {});

protected:
    void initApp() override;

    void initCamera();

    void createCube();

    void createCubeInstance(glm::vec3 color
                            ,  const glm::vec3& position = {0, 0, 0}
                            , const glm::vec3& scale = {1, 1, 1}
                            , const glm::quat& rotation = {1, 0, 0, 0});

    void createDescriptorPool();

    void randomCube();

    void createSkyBox();

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

protected:
    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } render;

    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } compute;
    SkyBox skyBox;

    std::unique_ptr<CameraController> cameraController;
    VulkanDescriptorPool descriptorPool;
    VulkanCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    VulkanPipelineCache pipelineCache;
    Entity cubeEntity;
    Entity cameraEntity;
    uint32_t cubeInstanceCount{0};
    struct Cube{};

    struct Color{
        glm::vec3 value;
    };

    struct InstanceData{
        glm::mat4 transform;
        glm::vec3 color;
    };
    Action* createInstanceAction;
};