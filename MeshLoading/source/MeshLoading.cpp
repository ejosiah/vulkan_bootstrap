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
    fmt::print("Vertex size: {}\n\tposition: {}\n\tcolor: {}\n\tnormal: {}\n\ttangent: {}\n\tbitangent: {}\n\tuv: {}"
               , sizeof(Vertex)
               , offsetOf(Vertex, position)
               , offsetOf(Vertex, color)
               , offsetOf(Vertex, normal)
               , offsetOf(Vertex, tangent)
               , offsetOf(Vertex, bitangent)
               , offsetOf(Vertex, uv)
    );
    return 0;
}