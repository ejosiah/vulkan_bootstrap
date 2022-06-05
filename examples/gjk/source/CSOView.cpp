#include "CSOView.hpp"

CSOView::CSOView(uint32_t w, uint32_t h, VulkanDevice *device, InputManager* inputMgr, FileManager *fileMgr)
: width{ w }
, height{ h }
, device{ device }
, inputManager{ inputMgr }
, fileManager{ fileMgr }
{
    createBuffers();
    createRenderTarget();
    createRenderPass();
    createFrameBuffer();
    createPipeline();
}

void CSOView::createRenderTarget() {
    auto usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    auto imageInfo = initializers::imageCreateInfo(VK_IMAGE_TYPE_2D, format, usage, width, height);
    renderTarget.image = device->createImage(imageInfo);

    auto subRange = DEFAULT_SUB_RANGE;
    renderTarget.imageView = renderTarget.image.createView(format, VK_IMAGE_VIEW_TYPE_2D, subRange);
}

void CSOView::createRenderPass() {

    std::vector<VkAttachmentDescription> attachments{
            {
                0,
                format,
                VK_SAMPLE_COUNT_1_BIT,
                VK_ATTACHMENT_LOAD_OP_CLEAR,
                VK_ATTACHMENT_STORE_OP_STORE,
                VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                VK_ATTACHMENT_STORE_OP_DONT_CARE,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            }
    };
    std::vector<SubpassDescription> subpasses(1);
    subpasses[0].colorAttachments.push_back({0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});

    std::vector<VkSubpassDependency> dependencies(1);
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[0].dstStageMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    renderPass = device->createRenderPass(attachments, subpasses, dependencies);
}

void CSOView::createFrameBuffer() {
    std::vector<VkImageView> attachments{ renderTarget.imageView };
    framebuffer = device->createFramebuffer(renderPass, attachments, width, height);
}

void CSOView::render() {
    device->graphicsCommandPool().oneTimeCommand([&](auto commandBuffer){

        VkClearValue clearValue{ 0.0f, 0.0f, 0.0f, 1.0f };
        VkRenderPassBeginInfo renderPassInfo = initializers::renderPassBeginInfo();
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = framebuffer;
        renderPassInfo.renderArea = { {0, 0}, {width, height}};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearValue;
        
        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkDeviceSize offset = 0;
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, cso.pipeline);
        // TODO camera
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, csoBuffer, &offset);
        vkCmdDraw(commandBuffer, csoBuffer.template sizeAs<glm::vec3>(), 1, 0, 0);
        
        
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, origin.pipeline);
        // TODO camera
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, originBuffer, &offset);
        vkCmdDraw(commandBuffer, 1, 1, 0, 0);
        
        vkCmdEndRenderPass(commandBuffer);
    });
}

void CSOView::createBuffers() {
    std::array<glm::vec3, 4> simplex{};
    csoBuffer = device->createCpuVisibleBuffer( simplex.data(), BYTE_SIZE(simplex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    glm::vec3 origin{0};
    originBuffer = device->createDeviceLocalBuffer(&origin, sizeof(glm::vec3), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

void CSOView::refreshRenderTarget() {
    device->graphicsCommandPool().oneTimeCommand([&](auto commandBuffer){
       renderTarget.image.transitionLayout(commandBuffer, VK_IMAGE_LAYOUT_UNDEFINED, DEFAULT_SUB_RANGE
                                           , VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
                                           , VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    });
}

void CSOView::update(std::vector<glm::vec3> &simplex) {
    assert(simplex.size() <= 4);
    auto vPtr = reinterpret_cast<glm::vec3*>(csoBuffer.map());
    for(int i = 0; i < simplex.size(); i++){
        vPtr[i] = simplex[i];
    }
}

void CSOView::createPipeline() {

}

void CSOView::update(float time) {

}

void CSOView::initCamera() {
    OrbitingCameraSettings settings{};
    settings.offsetDistance = 10.0f;
    settings.orbitMinZoom = 0.1f;
    settings.orbitMaxZoom = 10.f;
    settings.rotationSpeed = 0.1f;
    settings.zNear = 0.1f;
    settings.zFar = 20.0f;
    settings.fieldOfView = 45.0f;
    settings.modelHeight = 0;
    settings.aspectRatio = static_cast<float>(width)/static_cast<float>(height);
    cameraController = std::make_unique<OrbitingCameraController>( device, 1, 0, *inputManager, settings);
}
