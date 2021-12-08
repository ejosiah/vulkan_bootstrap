#include "SkeletalAnimationDemo.hpp"
#include "GraphicsPipelineBuilder.hpp"
#include "DescriptorSetBuilder.hpp"

SkeletalAnimationDemo::SkeletalAnimationDemo(const Settings& settings) : VulkanBaseApp("Skeletal Animation", settings) {
    fileManager.addSearchPath(".");
    fileManager.addSearchPath("../../examples/skeletal_animation");
    fileManager.addSearchPath("../../examples/skeletal_animation/spv");
    fileManager.addSearchPath("../../examples/skeletal_animation/models");
    fileManager.addSearchPath("../../examples/skeletal_animation/textures");
    fileManager.addSearchPath("../../data/shaders");
    fileManager.addSearchPath("../../data/models");
    fileManager.addSearchPath("../../data/textures");
    fileManager.addSearchPath("../../data");
}

void SkeletalAnimationDemo::initApp() {
    initModel();
    initCamera();
    createDescriptorPool();
    createCommandPool();
    createPipelineCache();
    createRenderPipeline();
    createComputePipeline();
}

void SkeletalAnimationDemo::createDescriptorPool() {
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

void SkeletalAnimationDemo::createCommandPool() {
    commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandBuffers = commandPool.allocateCommandBuffers(swapChainImageCount);
}

void SkeletalAnimationDemo::createPipelineCache() {
    pipelineCache = device.createPipelineCache();
}


void SkeletalAnimationDemo::createRenderPipeline() {
    auto builder = device.graphicsPipelineBuilder();
    render.pipeline =
        builder
            .shaderStage()
                .vertexShader(load("render.vert.spv"))
                .fragmentShader(load("render.frag.spv"))
            .vertexInputState()
                .addVertexBindingDescriptions(Vertex::bindingDisc())
                .addVertexAttributeDescriptions(Vertex::attributeDisc())
            .inputAssemblyState()
                .triangles()
            .viewportState()
                .viewport()
                    .origin(0, 0)
                    .dimension(swapChain.extent)
                    .minDepth(0)
                    .maxDepth(1)
                .scissor()
                    .offset(0, 0)
                    .extent(swapChain.extent)
                .add()
                .rasterizationState()
                    .cullBackFace()
                    .frontFaceCounterClockwise()
                    .polygonModeFill()
                .depthStencilState()
                    .enableDepthWrite()
                    .enableDepthTest()
                    .compareOpLess()
                    .minDepthBounds(0)
                    .maxDepthBounds(1)
                .colorBlendState()
                    .attachment()
                    .add()
                .renderPass(renderPass)
                .subpass(0)
                .name("render")
                .pipelineCache(pipelineCache)
            .build(render.layout);
}

void SkeletalAnimationDemo::createComputePipeline() {
    auto module = VulkanShaderModule{ "../../data/shaders/pass_through.comp.spv", device};
    auto stage = initializers::shaderStage({ module, VK_SHADER_STAGE_COMPUTE_BIT});

    compute.layout = device.createPipelineLayout();

    auto computeCreateInfo = initializers::computePipelineCreateInfo();
    computeCreateInfo.stage = stage;
    computeCreateInfo.layout = compute.layout;

    compute.pipeline = device.createComputePipeline(computeCreateInfo, pipelineCache);
}


void SkeletalAnimationDemo::onSwapChainDispose() {
    dispose(render.pipeline);
    dispose(compute.pipeline);
}

void SkeletalAnimationDemo::onSwapChainRecreation() {
    createRenderPipeline();
    createComputePipeline();
}

VkCommandBuffer *SkeletalAnimationDemo::buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) {
    numCommandBuffers = 1;
    auto& commandBuffer = commandBuffers[imageIndex];

    VkCommandBufferBeginInfo beginInfo = initializers::commandBufferBeginInfo();
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    static std::array<VkClearValue, 2> clearValues;
    clearValues[0].color = {0, 0, 1, 1};
    clearValues[1].depthStencil = {1.0, 0u};

    VkRenderPassBeginInfo rPassInfo = initializers::renderPassBeginInfo();
    rPassInfo.clearValueCount = COUNT(clearValues);
    rPassInfo.pClearValues = clearValues.data();
    rPassInfo.framebuffer = framebuffers[imageIndex];
    rPassInfo.renderArea.offset = {0u, 0u};
    rPassInfo.renderArea.extent = swapChain.extent;
    rPassInfo.renderPass = renderPass;

    vkCmdBeginRenderPass(commandBuffer, &rPassInfo, VK_SUBPASS_CONTENTS_INLINE);



    vkCmdEndRenderPass(commandBuffer);

    vkEndCommandBuffer(commandBuffer);

    return &commandBuffer;
}

void SkeletalAnimationDemo::update(float time) {
    VulkanBaseApp::update(time);
}

void SkeletalAnimationDemo::checkAppInputs() {
    VulkanBaseApp::checkAppInputs();
}

void SkeletalAnimationDemo::cleanup() {
    // TODO save pipeline cache
    VulkanBaseApp::cleanup();
}

void SkeletalAnimationDemo::onPause() {
    VulkanBaseApp::onPause();
}

void SkeletalAnimationDemo::initModel() {
//    model = Model::load("../../data/models/character/Wave_Hip_Hop_Dance.fbx");
}

void SkeletalAnimationDemo::initCamera() {
    CameraSettings settings{};
    settings.aspectRatio = static_cast<float>(swapChain.extent.width)/static_cast<float>(swapChain.extent.height);
    settings.rotationSpeed = 0.1f;
    settings.horizontalFov = true;
    settings.zNear = 1.0;
    settings.zFar = 100;
    cameraController = std::make_unique<CameraController>(device, swapChain.imageCount(), currentImageIndex, dynamic_cast<InputManager&>(*this), settings);
    cameraController->setMode(CameraMode::SPECTATOR);
    auto target = (model.bounds.max + model.bounds.min) * 0.5f;
    auto pos = target - glm::vec3(0, 0, 1);
    cameraController->lookAt(pos, target, {0, 1, 0});
}


int main(){
    try{

        Settings settings;
        settings.depthTest = true;

        auto app = SkeletalAnimationDemo{ settings };
//        app.run();
        std::shared_ptr<model::Model> m = model::load("../../data/models/character/Wave_Hip_Hop_Dance.fbx");
        for(auto& bone : m->bones){
            fmt::print("{}\n", bone.name);
        }
    }catch(std::runtime_error& err){
        spdlog::error(err.what());
    }
}