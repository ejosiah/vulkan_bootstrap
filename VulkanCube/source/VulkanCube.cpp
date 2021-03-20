#include "VulkanCube.h"

VulkanCube::VulkanCube():VulkanBaseApp("VulkanCube", 1080, 720, true){}

void VulkanCube::initApp() {

}

void VulkanCube::onSwapChainDispose() {
    VulkanBaseApp::onSwapChainDispose();
}

void VulkanCube::onSwapChainRecreation() {
}

void VulkanCube::createGraphicsPipeline() {
}

std::vector<VkCommandBuffer> VulkanCube::buildCommandBuffers(uint32_t i) {
    return VulkanBaseApp::buildCommandBuffers(i);
}

void VulkanCube::update(float time) {
    VulkanBaseApp::update(time);
}

void VulkanCube::createCommandBuffer() {
    VulkanBaseApp::createCommandBuffer();
}

void VulkanCube::createDescriptorPool() {
    VulkanBaseApp::createDescriptorPool();
}

void VulkanCube::createDescriptorSet() {
    VulkanBaseApp::createDescriptorSet();
}

void VulkanCube::createTextureBuffers() {
    VulkanBaseApp::createTextureBuffers();
}

void VulkanCube::createVertexBuffer() {
    VulkanBaseApp::createVertexBuffer();
}

void VulkanCube::createIndexBuffer() {
    VulkanBaseApp::createIndexBuffer();
}

