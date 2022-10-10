#include "VulkanRayTraceModel.hpp"
#include "VulkanRayTraceBaseApp.hpp"
#include "shader_binding_table.hpp"
#include "SkyBox.hpp"

class WhittedRayTracer : public VulkanRayTraceBaseApp {
public:
    explicit WhittedRayTracer(const Settings& settings = {});

protected:
    void initApp() override;

    void initCamera();

    void createModels();

    void createSkyBox();

    void createDescriptorPool();

    void createDescriptorSetLayouts();

    void updateDescriptorSets();

    void createCommandPool();

    void createPipelineCache();

    void initCanvas();

    void createInverseCam();

    void createRayTracingPipeline();

    void rayTrace(VkCommandBuffer commandBuffer);

    void rayTraceToCanvasBarrier(VkCommandBuffer commandBuffer) const;

    void CanvasToRayTraceBarrier(VkCommandBuffer commandBuffer) const;

    void onSwapChainDispose() override;

    void onSwapChainRecreation() override;

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override;

    void update(float time) override;

    void checkAppInputs() override;

    void cleanup() override;

    void onPause() override;

protected:
    struct {
        VulkanPipeline pipeline;
        VulkanPipelineLayout layout;
        VulkanDescriptorSetLayout descriptorSetLayout;
        VulkanDescriptorSetLayout instanceDescriptorSetLayout;
        VulkanDescriptorSetLayout vertexDescriptorSetLayout;
        VkDescriptorSet descriptorSet;
        VkDescriptorSet instanceDescriptorSet;
        VkDescriptorSet vertexDescriptorSet;
    } raytrace;

    ShaderTablesDescription shaderTablesDesc;
    ShaderBindingTables bindingTables;

    VulkanBuffer inverseCamProj;
    Canvas canvas{};
    Texture skybox;

    VulkanDescriptorPool descriptorPool;
    VulkanCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    VulkanPipelineCache pipelineCache;
    std::unique_ptr<OrbitingCameraController> camera;

    struct {
        std::vector<imp::Plane> planes;
        VulkanBuffer planeBuffer;
        rt::ImplicitObject asRef;
    } planes;

    struct {
        std::vector<imp::Sphere> spheres;
        VulkanBuffer buffer;
        rt::ImplicitObject asRef;
    } spheres[3];

    VulkanDescriptorSetLayout implicitObjectsDescriptorSetLayout;
    VkDescriptorSet implicitObjectsDescriptorSet;
};