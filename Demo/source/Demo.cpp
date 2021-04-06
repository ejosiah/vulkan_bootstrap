#include "Demo.h"

void Demo::initApp() {

}

void Demo::onSwapChainDispose() {
    VulkanBaseApp::onSwapChainDispose();
}

void Demo::onSwapChainRecreation() {
    VulkanBaseApp::onSwapChainRecreation();
}

VkCommandBuffer *Demo::buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) {
    return nullptr;
}

void Demo::update(float time) {
    VulkanBaseApp::update(time);
}

void Demo::checkAppInputs() {
    VulkanBaseApp::checkAppInputs();
}

void Demo::cleanup() {
    VulkanBaseApp::cleanup();
}
