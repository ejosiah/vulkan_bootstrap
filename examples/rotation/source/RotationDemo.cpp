#include "RotationDemo.hpp"
#include "GraphicsPipelineBuilder.hpp"
#include "DescriptorSetBuilder.hpp"
#include "RotationHierarchyScene.hpp"
#include "AngularMomentumScene.hpp"
#include "InterpolationScene.hpp"

RotationDemo::RotationDemo(const Settings& settings) : VulkanBaseApp("Rotation", settings) {
    fileManager.addSearchPath(".");
    fileManager.addSearchPath("../../examples/rotation");
    fileManager.addSearchPath("../../examples/rotation/spv");
    fileManager.addSearchPath("../../examples/rotation/models");
    fileManager.addSearchPath("../../examples/rotation/textures");
    fileManager.addSearchPath("../../data/shaders");
    fileManager.addSearchPath("../../data/shaders/phong");
    fileManager.addSearchPath("../../data/models");
    fileManager.addSearchPath("../../data/textures");
    fileManager.addSearchPath("../../data");
}

void RotationDemo::initApp() {
    createDescriptorPool();
    createCommandPool();
    createPipelineCache();
    createCamera();
    createScenes();

}

void RotationDemo::createDescriptorPool() {
    constexpr uint32_t maxSets = 100;
    std::array<VkDescriptorPoolSize, 16> poolSizes{
            {
                    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100 * maxSets},
                    {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100 * maxSets},
                    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100 * maxSets},
                    { VK_DESCRIPTOR_TYPE_SAMPLER, 100 * maxSets },
                    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100 * maxSets },
                    { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 100 * maxSets },
                    { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100 * maxSets },
                    { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 100 * maxSets },
                    { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 100 * maxSets },
                    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100 * maxSets },
                    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100 * maxSets },
                    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 100 * maxSets },
                    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 100 * maxSets },
                    { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 100 * maxSets },
                    { VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT, 100 * maxSets },
                    { VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 100 * maxSets }
            }
    };
    descriptorPool = device.createDescriptorPool(maxSets, poolSizes, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);

}

void RotationDemo::createCommandPool() {
    commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandBuffers = commandPool.allocateCommandBuffers(swapChainImageCount);
}

void RotationDemo::createPipelineCache() {
    pipelineCache = device.createPipelineCache();
}

void RotationDemo::onSwapChainDispose() {

}

void RotationDemo::onSwapChainRecreation() {
    cameraController->perspective(swapChain.aspectRatio());
    for(auto& [_, scene] : scenes){
        scene->refresh();
    }
}

VkCommandBuffer *RotationDemo::buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) {
    numCommandBuffers = 1;
    auto& commandBuffer = commandBuffers[imageIndex];

    VkCommandBufferBeginInfo beginInfo = initializers::commandBufferBeginInfo();
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    static std::array<VkClearValue, 2> clearValues;
    clearValues[0].color = {0.4, 0.4, 0.4, 1};
    clearValues[1].depthStencil = {1.0, 0u};

    VkRenderPassBeginInfo rPassInfo = initializers::renderPassBeginInfo();
    rPassInfo.clearValueCount = COUNT(clearValues);
    rPassInfo.pClearValues = clearValues.data();
    rPassInfo.framebuffer = framebuffers[imageIndex];
    rPassInfo.renderArea.offset = {0u, 0u};
    rPassInfo.renderArea.extent = swapChain.extent;
    rPassInfo.renderPass = renderPass;

    vkCmdBeginRenderPass(commandBuffer, &rPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    renderEntities(commandBuffer, scenes[currentScene]->m_registry);
    renderUI(commandBuffer);

    vkCmdEndRenderPass(commandBuffer);

    vkEndCommandBuffer(commandBuffer);

    return &commandBuffer;
}

void RotationDemo::update(float time) {

    if(moveCamera) {
        cameraController->update(time);
    }

    scenes[currentScene]->update(time);
}

void RotationDemo::checkAppInputs() {
    if(moveCamera) {
        cameraController->processInput();
    }
    scenes[currentScene]->checkInputs();
}

void RotationDemo::cleanup() {
   for(auto& [_, scene] : scenes){
       scene.get_deleter()(scene.get());
   }
}

void RotationDemo::onPause() {
    VulkanBaseApp::onPause();
}

void RotationDemo::createCamera() {
    OrbitingCameraSettings settings{};
    settings.offsetDistance = 3.5f;
    settings.rotationSpeed = 0.1f;
    settings.zNear = 0.1f;
    settings.zFar = 10.0f;
    settings.fieldOfView = 45.0f;
    settings.modelHeight = 0;
    settings.aspectRatio = static_cast<float>(swapChain.extent.width)/static_cast<float>(swapChain.extent.height);
    cameraController = std::make_unique<OrbitingCameraController>(dynamic_cast<InputManager&>(*this), settings);
    auto cameraEntity = Entity{m_registry };
    cameraEntity.add<component::Camera>().camera = const_cast<Camera*>(&cameraController->cam());
}

void RotationDemo::renderUI(VkCommandBuffer commandBuffer) {
    ImGui::Begin("Rotation");
    ImGui::SetWindowSize("Rotation", {300, 350});

    ImGui::Text("Scene:");
    ImGui::RadioButton("Rotation Hierarchy", &currentScene, 0);
    ImGui::RadioButton("Angular Momentum", &currentScene, 1);
    ImGui::RadioButton("Interpolation", &currentScene, 2);

    scenes[currentScene]->renderUI(commandBuffer);

    ImGui::Checkbox("Enable Camera", &moveCamera);
    ImGui::End();

    plugin(IM_GUI_PLUGIN).draw(commandBuffer);
}

void RotationDemo::createScenes() {
    scenes[0] = std::make_unique<RotationHierarchyScene>(&device, &renderPass, &swapChain, &descriptorPool, &fileManager, &mouse);
    scenes[1] = std::make_unique<AngularMomentumScene>(&device, &renderPass, &swapChain, &descriptorPool, &fileManager, &mouse);
    scenes[2] = std::make_unique<InterpolationScene>(&device, &renderPass, &swapChain, &descriptorPool, &fileManager, &mouse);

    for(auto& [_, scene] : scenes){
        scene->init();
        auto cameraEntity = Entity{ scene->m_registry };
        cameraEntity.add<component::Camera>().camera = const_cast<Camera*>(&cameraController->cam());
    }
}

int main(){
    try{

        Settings settings;
        settings.depthTest = true;
        settings.enabledFeatures.wideLines = true;

        auto app = RotationDemo{ settings };
        std::unique_ptr<Plugin> plugin = std::make_unique<ImGuiPlugin>();
        app.addPlugin(plugin);
        app.run();
    }catch(std::runtime_error& err){
        spdlog::error(err.what());
    }
}