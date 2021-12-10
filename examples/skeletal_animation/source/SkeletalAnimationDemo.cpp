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
    createDescriptorPool();
    createCommandPool();
    initModel();
    initCamera();
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
                .addVertexBindingDescription(1, sizeof(mdl::VertexBoneInfo), VK_VERTEX_INPUT_RATE_VERTEX)
                .addVertexAttributeDescriptions(Vertex::attributeDisc())
                .addVertexAttributeDescription(6, 1, VK_FORMAT_R32G32B32A32_UINT, (size_t)&(((mdl::VertexBoneInfo*)0)->boneIds[0]))
                .addVertexAttributeDescription(7, 1, VK_FORMAT_R32G32B32A32_UINT, (size_t)&(((mdl::VertexBoneInfo*)0)->boneIds[4]))
                .addVertexAttributeDescription(8, 1, VK_FORMAT_R32G32B32A32_SFLOAT, (size_t)&(((mdl::VertexBoneInfo*)0)->weights[0]))
                .addVertexAttributeDescription(9, 1, VK_FORMAT_R32G32B32A32_SFLOAT, (size_t)&(((mdl::VertexBoneInfo*)0)->weights[4]))
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
                .layout()
                    .addPushConstantRange(Camera::pushConstant())
                    .addDescriptorSetLayout(model->descriptor.setLayout)
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

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, render.pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, render.layout, 0, 1, &model->descriptor.set, 0, VK_NULL_HANDLE);
    cameraController->push(commandBuffer, render.layout);
    model->render(commandBuffer);

    vkCmdEndRenderPass(commandBuffer);

    vkEndCommandBuffer(commandBuffer);

    return &commandBuffer;
}

void SkeletalAnimationDemo::update(float time) {
    animation.update(time);
    cameraController->update(time);
}

void SkeletalAnimationDemo::checkAppInputs() {
    cameraController->processInput();
}

void SkeletalAnimationDemo::cleanup() {
    model->buffers.boneTransforms.unmap();
    // TODO save pipeline cache
    VulkanBaseApp::cleanup();
}

void SkeletalAnimationDemo::onPause() {
    VulkanBaseApp::onPause();
}

void walkBoneHierarchy1(const anim::Animation& animation, const anim::AnimationNode& node, int depth){
    const auto& nodes = animation.nodes;
    fmt::print("{: >{}}{}[{}, {}]\n", "", depth + 1, node.name, node.id, node.parentId);
    for(const auto& i : node.children){
        walkBoneHierarchy1(animation, nodes[i], depth + 1);
    }
}

void SkeletalAnimationDemo::initModel() {
    auto path = std::string{ "../../data/models/character/Wave_Hip_Hop_Dance.fbx" };
    model = mdl::load(device, path);
    model->updateDescriptorSet(device, descriptorPool);
    animation = anim::load(model.get(), path).front();
    spdlog::info("model bounds: [{}, {}], height: {}", model->bounds.min, model->bounds.max, model->height());
    walkBoneHierarchy1(animation, animation.nodes[0], 0);
}

void SkeletalAnimationDemo::initCamera() {
    OrbitingCameraSettings settings{};
    settings.aspectRatio = static_cast<float>(swapChain.extent.width)/static_cast<float>(swapChain.extent.height);
    settings.rotationSpeed = 0.1f;
    settings.horizontalFov = true;
    settings.offsetDistance = 900.f;
    settings.orbitMaxZoom = settings.offsetDistance;
    settings.modelHeight = model->height() - model->bounds.min.y;
    settings.model.min = model->bounds.min;
    settings.model.max = model->bounds.max;
    cameraController = std::make_unique<OrbitingCameraController>(device, swapChain.imageCount(), currentImageIndex, dynamic_cast<InputManager&>(*this), settings);

}

void walkBoneHierarchy(const mdl::Model& model, const mdl::Bone& bone, int depth){
    const std::vector<mdl::Bone>& bones = model.bones;
    auto [min, max] = model.boneBounds[bone.id];
    fmt::print("{: >{}}{}[min: {}, max: {}]\n", "", depth + 1, bone.name, min, max);
    for(const auto& i : bone.children){
        walkBoneHierarchy(model, bones[i], depth + 1);
    }
}

int main(){
    try{

        Settings settings;
        settings.depthTest = true;

        auto app = SkeletalAnimationDemo{ settings };
        app.run();
//        std::string path = "../../data/models/character/Wave_Hip_Hop_Dance.fbx";
//        std::shared_ptr<model::Model> m = model::load(path);
//        auto animation = anim::load(m.get(), path).front();
//        fmt::print("animation:\n");
//        fmt::print("\tname: {}\n", animation.name);
//        fmt::print("\tduration: {}\n", animation.duration);
//        fmt::print("\tticksPerSecond: {}\n", animation.ticksPerSecond);
//        fmt::print("\tchannels: {}\n", animation.channels.size());
//        walkBoneHierarchy(*m, m->bones[m->rootBone], 0);
//        fmt::print("\n\n");
////        for(auto& bone : m->bones){
////            fmt::print("{}\n", bone.name);
////        }
//    auto duration = animation.duration/animation.ticksPerSecond;
//    fmt::print("\tduration in seconds: {}\n", duration);
//
////    for(float t = 0; t < 17; t += 0.3f){
////        auto tick = std::fmod((t * animation.ticksPerSecond), animation.duration);
////        fmt::print("tick: {}\n", tick);
////    }
//    fmt::print("\n\n");
//
////    for(int i = 0; i < animation.channels.size(); i++){
////        const auto channel = animation.channels[i];
////        fmt::print("channel[name: {}, id: {}]\n", channel.name, i);
////        for(auto j = 0; j < channel.translationKeys.size(); j++){
////            auto key = channel.translationKeys[j];
////            if(key.time > 0) {
////                fmt::print("\tkey[id: {}, time: {}]\n", j, key.time);
////            }
////        }
////        fmt::print("\n\n");
////    }
//    auto leftIndexFinger = m->bones[49];
//    fmt::print("name: {}, parent: {}", leftIndexFinger.name, m->bones[leftIndexFinger.parent].name);

    }catch(std::runtime_error& err){
        spdlog::error(err.what());
    }
}