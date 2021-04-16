#include <ImGuiPlugin.hpp>
#include "FontTest.h"
#include "glm_format.h"

FontTest::FontTest() :VulkanBaseApp("Font Test") {
    cappedSink = std::make_shared<VulkanSink<spdlog::details::null_mutex, 20>>();
    spdlog::default_logger()->sinks().push_back(cappedSink);
}

void FontTest::initApp() {
    commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandBuffers = commandPool.allocate(swapChain.imageCount());
}


void FontTest::update(float time) {

}


void FontTest::onSwapChainDispose() {

}

void FontTest::onSwapChainRecreation() {

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


    auto& imGuiPlugin = plugin<ImGuiPlugin>(IM_GUI_PLUGIN);
 //   auto font = imGuiPlugin.font("Arial", 20);
  //  ImGui::PushFont(font);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMouseInputs | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground;
    ImGui::Begin("Font test", nullptr, flags);
    ImGui::SetWindowSize({float(width), float(height)});
    ImGui::TextColored({1, 1, 0, 1}, "Frames: %llu\nFPS: %d", frameCount, framePerSecond);
    ImGui::TextColored({1, 1, 0, 1}, "Hello World, And well come to the world of Vulkan!");
    std::for_each(cappedSink->begin(), cappedSink->end(), [&](const std::string& msg){
        if(msg.find("error") != std::string::npos){
            ImGui::TextColored({1, 0, 0, 1}, msg.c_str());
        }else {
            ImGui::TextColored({1, 1, 0, 1}, msg.c_str());
        }
    });
    ImGui::End();
//    ImGui::PopFont();


    imGuiPlugin.draw(commandBuffer);

    vkCmdEndRenderPass(commandBuffer);
    vkEndCommandBuffer(commandBuffer);

    return &commandBuffer;
}

void FontTest::cleanup() {
    VulkanBaseApp::cleanup();
}

int main(){
    try{
        std::vector<FontInfo> fonts {
                {"JetBrainsMono", R"(C:\Users\Josiah\Downloads\JetBrainsMono-2.225\fonts\ttf\JetBrainsMono-Regular.ttf)", 20},
                {"Arial", R"(C:\Windows\Fonts\arial.ttf)", 20}
        };
        std::unique_ptr<Plugin> imGui = std::make_unique<ImGuiPlugin>(fonts);
        auto app = FontTest();
        app.addPlugin(imGui);
        app.run();
    }catch(const std::runtime_error& err){
        spdlog::error(err.what());
    }
    return 0;
}