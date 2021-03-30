#define GLM_FORCE_SWIZZLE
#include "MeshLoading.h"
#include "glm_format.h"

MeshLoading::MeshLoading() : VulkanBaseApp("Coordinate Systems"){}

void MeshLoading::initApp() {
    commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandBuffers = commandPool.allocate(swapChain.imageCount());
}

void MeshLoading::onSwapChainDispose() {
    dispose(commandPool);
}

void MeshLoading::onSwapChainRecreation() {
    VulkanBaseApp::onSwapChainRecreation();
}

VkCommandBuffer *MeshLoading::buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) {
    auto& commandBuffer = commandBuffers[imageIndex];

    VkCommandBufferBeginInfo beginInfo = initializers::commandBufferBeginInfo();
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkClearValue clearValue{0.0f, 1.0f, 0.0f, 1.0f};

    VkRenderPassBeginInfo beginRenderPass = initializers::renderPassBeginInfo();
    beginRenderPass.renderPass = renderPass;
    beginRenderPass.framebuffer = framebuffers[imageIndex];
    beginRenderPass.renderArea.extent = swapChain.extent;
    beginRenderPass.renderArea.offset = {0, 0};
    beginRenderPass.clearValueCount = 1.0f;
    beginRenderPass.pClearValues = &clearValue;

    vkCmdBeginRenderPass(commandBuffer, &beginRenderPass, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdEndRenderPass(commandBuffer);
    vkEndCommandBuffer(commandBuffer);

    numCommandBuffers = 1;
    return &commandBuffer;

}

void MeshLoading::update(float time) {
    VulkanBaseApp::update(time);
}

void MeshLoading::checkAppInputs() {
    VulkanBaseApp::checkAppInputs();
}


int main(){
    std::vector<mesh::Mesh> meshes;
    mesh::load(meshes, R"(C:\Users\Josiah\OneDrive\media\models\ChineseDragon.obj)");
    return 0;
}