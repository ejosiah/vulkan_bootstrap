#include <ImGuiPlugin.hpp>
#include "FontTest.h"
#include "glm_format.h"

FontTest::FontTest() :VulkanBaseApp("Font Test") {

}

void FontTest::initApp() {
    commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandBuffers = commandPool.allocate(swapChain.imageCount());
    std::stringstream ss;
    ss.clear();
    ss.str("");
    ss
            << "Press 1 to switch to first person behavior\n"
            << "Press 2 to switch to spectator behavior\n"
            << "Press 3 to switch to flight behavior\n"
            << "Press 4 to switch to orbit behavior\n\n"
            << "First Person and Spectator behaviors\n"
            << "\tPress W and S to move forwards and backwards\n"
            << "\tPress A and D to strafe left and right\n"
            << "\tPress E and Q to move up and down\n"
            << "\tMove mouse to free look\n\n"
            << "Flight behavior\n"
            << "\tPress W and S to move forwards and backwards\n"
            << "\tPress A and D to yaw left and right\n"
            << "\tPress E and Q to move up and down\n"
            << "\tMove mouse up and down to change pitch\n"
            << "\tMove mouse left and right to change roll\n\n"
            << "Orbit behavior\n"
            << "\tPress SPACE to enable/disable target Y axis orbiting\n"
            << "\tMove mouse to orbit the model\n"
            << "\tMouse wheel to zoom in and out\n\n"
            << "Press M to enable/disable mouse smoothing\n"
            << "Press V to enable/disable vertical sync\n"
            << "Press + and - to change camera rotation speed\n"
            << "Press , and . to change mouse sensitivity\n"
            << "Press BACKSPACE or middle mouse button to level camera\n"
            << "Press ALT and ENTER to toggle full screen\n"
            << "Press ESC to exit\n\n"
            << "Press H to hide help";
;

    msg = ss.str();
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
    auto font = imGuiPlugin.font("Arial", 20);
    ImGui::PushFont(font);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMouseInputs | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground;
    ImGui::Begin("Font test", nullptr, flags);
    ImGui::SetWindowSize({float(width), float(height)});
    ImGui::TextColored({1, 1, 0, 1}, "Frames: %llu\nFPS: %d", frameCount, framePerSecond);
    ImGui::TextColored({1, 1, 0, 1}, "Hello World, And well come to the world of Vulkan!");
    ImGui::End();
    ImGui::PopFont();


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