#pragma once
#include "VulkanBaseApp.h"
#include "BulletPhysicsPlugin.hpp"
#include "SkyBox.hpp"
#include "VulkanRayTraceModel.hpp"
#include "VulkanRayQuerySupport.hpp"
constexpr float     CAMERA_FOVX = 90;
constexpr float     CAMERA_ZFAR = 100.0f;
constexpr float     CAMERA_ZNEAR = 0.1f;
constexpr float     CAMERA_ZOOM_MAX = 5.0f;
constexpr float     CAMERA_ZOOM_MIN = 1.5f;
constexpr glm::vec3   CAMERA_ACCELERATION(4.0f, 4.0f, 4.0f);
constexpr glm::vec3   CAMERA_VELOCITY(1.0f, 1.0f, 1.0f);

struct FloorVertexData{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

struct VertexData{
    glm::vec3 position;
    glm::vec3 normal;
};

struct VertexInstanceData{
    glm::mat4 xform;
    glm::vec3 color;
};


class BulletPhysicsDemo : public VulkanBaseApp, public VulkanRayQuerySupport{
public:
    explicit BulletPhysicsDemo(const Settings& settings = {});

protected:
    void initApp() override;

    void initCamera();

    void createCubes();

    void createDescriptorPool();

    void createDescriptorSetLayouts();

    void createSkyBox();

    void createCommandPool();

    void createPipelineCache();

    void createRenderPipeline();

    void createComputePipeline();

    void onSwapChainDispose() override;

    void onSwapChainRecreation() override;

    void createRigidBodies();

    void createAccelerationStructure(const std::vector<RigidBody>& rigidBodies);

    void createDescriptorSets();

    void updateAccelerationStructureDescriptorSet();

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override;

    void drawCubes(VkCommandBuffer commandBuffer);

    void drawSkyBox(VkCommandBuffer commandBuffer);

    void drawFloor(VkCommandBuffer commandBuffer);

    void update(float time) override;

    void checkAppInputs() override;

    void cleanup() override;

    void onPause() override;

    void calculateCameraShade();

    void displayInfo(VkCommandBuffer commandBuffer);

    static inline glm::vec3 trilerp(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d
                          , glm::vec3 e, glm::vec3 f, glm::vec3 g, glm::vec3 h, glm::vec3 t){
        float u = t.x;
        float v = t.y;
        float w = t.z;
        return glm::mix(
                glm::mix( glm::mix(a, b, u), glm::mix(c, d, u), v),
                glm::mix(glm::mix(e, f, u), glm::mix(g, h, u), v),
                w
                );
    }

protected:
    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } render;

    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
        VulkanBuffer vertexBuffer;
        VulkanBuffer indexBuffer;
    } floor;

    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } compute;

    VulkanDescriptorPool descriptorPool;
    VulkanCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    VulkanPipelineCache pipelineCache;
    std::unique_ptr<CameraController> cameraController;

    glm::vec3 lightDir{3, 7, 6};
    SkyBox skyBox;
    float shake = 0;
    float trauma = 0;

    struct{
        VulkanBuffer vertexBuffer;
        VulkanBuffer indexBuffer;
        VulkanBuffer instanceBuffer;
        std::vector<RigidBodyId> ids;
        std::vector<glm::mat4> transforms;
        uint32_t numBoxes{125};
    } cubes{};

    VulkanDescriptorSetLayout accStructDescriptorSetLayout;
    VkDescriptorSet accStructDescriptorSet;
    std::vector<rt::Instance> asInstances;
    rt::AccelerationStructureBuilder accStructBuilder;
};