$if(raytracing.truthy)$
#include "VulkanRayTraceModel.hpp"
#include "VulkanRayTraceBaseApp.hpp"
#include "shader_binding_table.hpp"

class $classname$ : public VulkanRayTraceBaseApp {
$else$
#include "VulkanBaseApp.h"

class $classname$ : public VulkanBaseApp{
$endif$
public:
    explicit $classname$(const Settings& settings = {});

protected:
    void initApp() override;

    void initCamera();

    void createDescriptorPool();

    void createDescriptorSetLayouts();

    void updateDescriptorSets();

    void createCommandPool();

    void createPipelineCache();

    $if(raytracing.truthy)$
    void initCanvas();

    void createInverseCam();

    void createRayTracingPipeline();

    void rayTrace(VkCommandBuffer commandBuffer);

    void rayTraceToCanvasBarrier(VkCommandBuffer commandBuffer) const;

    void CanvasToRayTraceBarrier(VkCommandBuffer commandBuffer) const;
    $endif$

    $if(render.truthy)$
    void createRenderPipeline();
    $endif$

    $if(compute.truthy)$
    void createComputePipeline();
    $endif$

    void onSwapChainDispose() override;

    void onSwapChainRecreation() override;

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override;

    void update(float time) override;

    void checkAppInputs() override;

    void cleanup() override;

    void onPause() override;

protected:
    $if(render.truthy)$
    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } render;
    $endif$

    $if(compute.truthy)$
    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } compute;
    $endif$

    $if(raytracing.truthy)$
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
    $endif$

    VulkanDescriptorPool descriptorPool;
    VulkanCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    VulkanPipelineCache pipelineCache;
    std::unique_ptr<OrbitingCameraController> camera;
};