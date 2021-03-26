#include "FontTest.h"
#include "glm_format.h"

FontTest::FontTest() :VulkanBaseApp("Font Test", 2048, 702) {
    enabledFeatures.fillModeNonSolid = VK_TRUE;
    enabledFeatures.wideLines = VK_TRUE;
}

void FontTest::initApp() {
    Fonts::init(&device, &renderPass, swapChain.imageCount(), &currentImageIndex, width, height);
    font = Fonts::getFont(ARIAL, 20, FontStyle::NORMAL, {1, 1, 0});
    commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandBuffers = commandPool.allocate(swapChain.imageCount());

}


void FontTest::update(float time) {
    font->clear();
    font->write(fmt::format("Frames: {}\nFPS: {}", frameCount, framePerSecond), 20, 50);
    font->write("Hello World, And well come to the world of Vulkan!", 20, 100);
}


void FontTest::onSwapChainDispose() {

}

void FontTest::onSwapChainRecreation() {
    Fonts::refresh(width, height, &renderPass);
}

VkCommandBuffer *FontTest::buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) {
    numCommandBuffers = 1;
    auto& commandBuffer = commandBuffers[imageIndex];
    auto beginInfo = initializers::commandBufferBeginInfo();

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkClearValue clear{0.0f, 0.0f, 0.0f, 0.0f};
    auto renderPassBeginInfo = initializers::renderPassBeginInfo();
    renderPassBeginInfo.renderPass = renderPass;
    renderPassBeginInfo.framebuffer = framebuffers[imageIndex];
    renderPassBeginInfo.renderArea.offset = { 0, 0};
    renderPassBeginInfo.renderArea.extent = swapChain.extent;
    renderPassBeginInfo.clearValueCount = 1u;
    renderPassBeginInfo.pClearValues = &clear;
    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    font->draw(commandBuffer);

    vkCmdEndRenderPass(commandBuffer);
    vkEndCommandBuffer(commandBuffer);

    return &commandBuffer;
}

void FontTest::cleanup() {
    Fonts::cleanup();
    VulkanBaseApp::cleanup();
}

int main(){
    try{
        auto app = FontTest();
        app.run();
    }catch(const std::runtime_error& err){
        spdlog::error(err.what());
    }
    return 0;
}