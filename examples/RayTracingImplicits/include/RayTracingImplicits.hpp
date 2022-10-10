#pragma once

#include "VulkanRayTraceBaseApp.hpp"
#include "shader_binding_table.hpp"

class RayTracingImplicits : public VulkanRayTraceBaseApp {
public:
    explicit RayTracingImplicits(const Settings& settings);

protected:
    void initApp() override;

    void createModel();

    void createPlanes();

    void createSpheres();

    void initCamera();

    void createCommandPool();

    void createDescriptorPool();

    void createDescriptorSetLayout();

    void createDescriptorSet();

    void createPipeline();

    void createBindingTables();

    void onSwapChainDispose() override;

    void onSwapChainRecreation() override;

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override;

    void rayTracingToGraphicsBarrier(VkCommandBuffer commandBuffer);

    void GraphicsToRayTracingBarrier(VkCommandBuffer commandBuffer);

    void rayTrace(VkCommandBuffer commandBuffer);

    VkImageMemoryBarrier memoryBarrier(VkAccessFlags srcAccessMark, VkAccessFlags dstAccessMark);

    void update(float time) override;

    void checkAppInputs() override;

    void cleanup() override;

private:
    VulkanCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    VulkanDescriptorPool descriptorPool;
    VulkanDescriptorSetLayout descriptorSetLayout;
    VulkanDescriptorSetLayout objectsDescriptorSetLayout;
    VkDescriptorSet descriptorSet;
    VkDescriptorSet objectsDescriptorSet;
    VulkanPipelineLayout layout;
    VulkanPipeline pipeline;
    VulkanPipelineCache pipelineCache;

    std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups;

    struct {
        std::vector<imp::Plane> planes;
        VulkanBuffer planeBuffer;
        rt::ImplicitObject asRef;
    } planes;

    struct {
        std::vector<imp::Sphere> spheres;
        VulkanBuffer buffer;
        rt::ImplicitObject asRef;
    } spheres;

    struct Cam{
        glm::mat4 view;
        glm::mat4 projection;
    } cam;

    ShaderTablesDescription shaderTablesDesc;
    ShaderBindingTables bindingTables;

    std::unique_ptr<OrbitingCameraController> camera;
};