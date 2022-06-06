#pragma once

#include "VulkanBaseApp.h"
#include "VulkanDevice.h"
#include "filemanager.hpp"

class CSOView{
public:
    CSOView() = default;

    CSOView(uint32_t w, uint32_t  h, VulkanDevice* device, InputManager* inputMgr, FileManager* fileMgr);

    void createRenderTarget();

    void createRenderPass();

    void createFrameBuffer();

    void createBuffers();

    void createPipeline();

    void update(std::vector<glm::vec3>& simplex, std::vector<uint16_t>& indices);

    void render();

    void update(float time);

    void initCamera();

    void checkAppInput();

    FramebufferAttachment renderTarget;

private:
    VulkanDevice* device{nullptr };
    FileManager* fileManager{ nullptr };
    InputManager* inputManager{nullptr };
    VulkanRenderPass renderPass;
    VulkanFramebuffer framebuffer;
    VulkanBuffer csoBuffer;
    VulkanBuffer csoIndexBuffer;
    VulkanBuffer originBuffer;
    VulkanBuffer gridBuffer;
    uint32_t simplexSize{0};
    uint32_t width{0}, height{0};
    VkFormat format{ VK_FORMAT_R8G8B8A8_UNORM };
    std::unique_ptr<OrbitingCameraController> cameraController;

    struct{
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } cso;

    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } origin;

    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } grid;
};