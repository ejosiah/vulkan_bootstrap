#include "VulkanBaseApp.h"
#include "VulkanDevice.h"
#include "filemanager.hpp"

#pragma once

class CSOView{
public:
    CSOView() = default;

    CSOView(uint32_t w, uint32_t  h, VulkanDevice* device, InputManager* inputMgr, FileManager* fileMgr);

    void createRenderTarget();

    void createRenderPass();

    void createFrameBuffer();

    void createBuffers();

    void createPipeline();

    void update(std::vector<glm::vec3>& simplex);

    void render();

    void refreshRenderTarget();

    void update(float time);

    void initCamera();

private:
    VulkanDevice* device{nullptr };
    FileManager* fileManager{ nullptr };
    InputManager* inputManager{nullptr };
    VulkanRenderPass renderPass;
    VulkanFramebuffer framebuffer;
    FramebufferAttachment renderTarget;
    VulkanBuffer csoBuffer;
    VulkanBuffer originBuffer;
    uint32_t width{0}, height{0};
    VkFormat format{ VK_FORMAT_R8G8B8A8_UNORM };
    std::unique_ptr<OrbitingCameraController> cameraController;

    struct{
        VulkanPipelineLayout pipelineLayout;
        VulkanPipeline pipeline;
    } cso;

    struct {
        VulkanPipelineLayout pipelineLayout;
        VulkanPipeline pipeline;
    } origin;
};