#include "ImGuiDemo.hpp"

ImGuiDemo::ImGuiDemo(const Settings& settings) :VulkanBaseApp("ImGui Demo", settings) {}

void ImGuiDemo::initApp() {
    commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandBuffers = commandPool.allocateCommandBuffers(swapChainImageCount);
}

void ImGuiDemo::onSwapChainDispose() {
    VulkanBaseApp::onSwapChainDispose();
}

void ImGuiDemo::onSwapChainRecreation() {
    VulkanBaseApp::onSwapChainRecreation();
}

VkCommandBuffer *ImGuiDemo::buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) {
    numCommandBuffers = 1;
    auto& commandBuffer = commandBuffers[imageIndex];

    auto beginInfo = initializers::commandBufferBeginInfo();
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkClearValue clearValue{};
    clearValue.color.float32[0] = clearColor.r * clearColor.a;
    clearValue.color.float32[1] = clearColor.g * clearColor.a;
    clearValue.color.float32[2] = clearColor.b * clearColor.a;
    clearValue.color.float32[3] = clearColor.a;

    auto renderPassInfo = initializers::renderPassBeginInfo();
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = framebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChain.extent;
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearValue;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    if(show_demo_window) {
        ImGui::ShowDemoWindow(&show_demo_window);
    }

    static float f = 0.0f;
    static int counter = 0;

    ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

    ImGui::Text("This is some useful text.\nanother useful text");               // Display some text (you can use a format strings too)
    ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
    ImGui::Checkbox("Another Window", &show_another_window);

    ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
    ImGui::ColorEdit3("clear color", &clearColor.x); // Edit 3 floats representing a color

    if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
        counter++;
    ImGui::SameLine();
    ImGui::Text("counter = %d", counter);

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();

    // 3. Show another simple window.
    if (show_another_window){
        ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me"))
            show_another_window = false;
        ImGui::End();
    }

    auto& imGui = plugin(IM_GUI_PLUGIN);
    imGui.draw(commandBuffer);

    vkCmdEndRenderPass(commandBuffer);
    vkEndCommandBuffer(commandBuffer);

    return &commandBuffer;
}



int main(){
    try{
        std::unique_ptr<Plugin> plugin = std::make_unique<ImGuiPlugin>();
//        std::vector<std::unique_ptr<Plugin>> plugins;
//        plugins.push_back(std::move(plugin));
        Settings settings;
      //  settings.vSync = true;
        auto app = ImGuiDemo{settings};
        app.addPlugin(plugin);
        app.run();
    }catch(std::runtime_error& err){
        spdlog::info(err.what());
    }
}