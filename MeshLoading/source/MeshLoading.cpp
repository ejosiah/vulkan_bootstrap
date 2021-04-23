#include "MeshLoading.h"
#include "glm_format.h"
#include "xforms.h"

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
    glm::vec3 y{0, 1, 0};
    glm::vec3 x{1, 0, 0};
    glm::vec3 n_y{0, -1, 0};
    glm::vec3 n_x{-1, 0, 0};
    fmt::print("{}\n", glm::cross(x, y));
    fmt::print("{}\n", glm::cross(n_y, x));
    fmt::print("{}\n", glm::cross(n_x, n_y));
    fmt::print("{}\n", glm::cross(y, n_x));
    return 0;
}