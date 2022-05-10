#pragma once

#include "VulkanBaseApp.h"
#include "Texture.h"

class BRDFGenerator{
public:
    BRDFGenerator(VulkanDevice* device = VK_NULL_HANDLE, FileManager* fileManager = VK_NULL_HANDLE);

    void init();

    void createRenderPass();

    void createFramebuffer();

    void createFramebufferAttachment();

    void createPipeline();

    void createScreenVertices();

    Texture generate();

private:
    VulkanDevice* device;
    VulkanRenderPass renderPass;
    VulkanFramebuffer framebuffer;
    VulkanPipeline pipeline;
    VulkanPipelineLayout layout;
    FileManager* fileManager;
    VulkanBuffer screenVertices;
    FramebufferAttachment colorAttachment;
};