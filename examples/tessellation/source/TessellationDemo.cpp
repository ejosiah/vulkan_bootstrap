#include "TessellationDemo.hpp"
#include "GraphicsPipelineBuilder.hpp"
#include "DescriptorSetBuilder.hpp"
#include "ImGuiPlugin.hpp"

TessellationDemo::TessellationDemo(const Settings& settings) : VulkanBaseApp("Tessellation", settings) {
    fileManager.addSearchPath(".");
    fileManager.addSearchPath("../../examples/tessellation");
    fileManager.addSearchPath("../../examples/tessellation/spv");
    fileManager.addSearchPath("../../examples/tessellation/models");
    fileManager.addSearchPath("../../examples/tessellation/textures");
    fileManager.addSearchPath("../../data/shaders");
    fileManager.addSearchPath("../../data/models");
    fileManager.addSearchPath("../../data/textures");
    fileManager.addSearchPath("../../data");
}

void TessellationDemo::initApp() {
    createDescriptorPool();
    createCommandPool();
    createPatches();
    initCamera();
    createPipelineCache();
    createRenderPipeline();
    createComputePipeline();
}
void TessellationDemo::createDescriptorPool() {
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

void TessellationDemo::createCommandPool() {
    commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandBuffers = commandPool.allocateCommandBuffers(swapChainImageCount);
}

void TessellationDemo::createPipelineCache() {
    pipelineCache = device.createPipelineCache();
}


void TessellationDemo::createRenderPipeline() {
    auto builder = device.graphicsPipelineBuilder();
    quad.equalSpacing.pipeline =
        builder
            .allowDerivatives()
            .shaderStage()
                .vertexShader(load("patch.vert.spv"))
                .tessellationControlShader(load("quad.tesc.spv"))
                .tessellationEvaluationShader(load("quad.tese.spv"))
                .fragmentShader(load("patch.frag.spv"))
            .vertexInputState()
                .addVertexBindingDescription(0, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX)
                .addVertexAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0)
            .inputAssemblyState()
                .patches()
            .tessellationState()
                .patchControlPoints(4)
                .domainOrigin(VK_TESSELLATION_DOMAIN_ORIGIN_LOWER_LEFT)
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
                    .cullNone()
                    .frontFaceCounterClockwise()
                    .polygonModeLine()
                .multisampleState()
                    .rasterizationSamples(settings.msaaSamples)
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
                    .addPushConstantRange(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, 0, sizeof(quad.constants))
                .renderPass(renderPass)
                .subpass(0)
                .name("quad_equal_spacing")
                .pipelineCache(pipelineCache)
            .build(quad.equalSpacing.layout);

    quad.fractionalEvenSpacing.pipeline =
        builder
            .basePipeline(quad.equalSpacing.pipeline)
            .shaderStage()
                .tessellationEvaluationShader(load("quad_even.tese.spv"))
            .name("quand_even_spacing")
        .build(quad.fractionalEvenSpacing.layout);

    quad.fractionalOddSpacing.pipeline =
        builder
            .basePipeline(quad.equalSpacing.pipeline)
            .shaderStage()
                .tessellationEvaluationShader(load("quad_odd.tese.spv"))
            .name("quad_odd_spacing")
        .build(quad.fractionalOddSpacing.layout);

    tri.equalSpacing.pipeline =
        builder
            .allowDerivatives()
            .basePipeline(quad.equalSpacing.pipeline)
            .shaderStage()
                .tessellationControlShader(load("triangle.tesc.spv"))
                .tessellationEvaluationShader(load("triangle.tese.spv"))
            .rasterizationState()
                .cullNone()
            .tessellationState()
                .patchControlPoints(3)
                .domainOrigin(VK_TESSELLATION_DOMAIN_ORIGIN_LOWER_LEFT)
            .layout().clear()
                .addPushConstantRange(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, 0, sizeof(tri.constants))
            .name("triangle_equal_spacing")
        .build(tri.equalSpacing.layout);


    tri.fractionalEvenSpacing.pipeline =
        builder
            .basePipeline(tri.equalSpacing.pipeline)
            .shaderStage()
                .tessellationEvaluationShader(load("triangle_even.tese.spv"))
            .name("triangle_even_spacing")
        .build(tri.fractionalEvenSpacing.layout);

    tri.fractionalOddSpacing.pipeline =
        builder
            .basePipeline(tri.equalSpacing.pipeline)
            .shaderStage()
                .tessellationEvaluationShader(load("triangle_odd.tese.spv"))
            .name("triangle_odd_spacing")
        .build(tri.fractionalOddSpacing.layout);

    isoline.equalSpacing.pipeline =
        builder
            .basePipeline(quad.equalSpacing.pipeline)
            .allowDerivatives()
            .shaderStage()
                .tessellationControlShader(load("isolines.tesc.spv"))
                .tessellationEvaluationShader(load("isolines.tese.spv"))
            .tessellationState()
                .patchControlPoints(4)
                .domainOrigin(VK_TESSELLATION_DOMAIN_ORIGIN_LOWER_LEFT)
            .layout().clear()
                .addPushConstantRange(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, 0, sizeof(isoline.constants))
            .name("isolines_equal_spacing")
        .build(isoline.equalSpacing.layout);



}

void TessellationDemo::createComputePipeline() {
    auto module = VulkanShaderModule{ "../../data/shaders/pass_through.comp.spv", device};
    auto stage = initializers::shaderStage({ module, VK_SHADER_STAGE_COMPUTE_BIT});

    compute.layout = device.createPipelineLayout();

    auto computeCreateInfo = initializers::computePipelineCreateInfo();
    computeCreateInfo.stage = stage;
    computeCreateInfo.layout = compute.layout;

    compute.pipeline = device.createComputePipeline(computeCreateInfo, pipelineCache);
}


void TessellationDemo::onSwapChainDispose() {
    dispose(quad.equalSpacing.pipeline);
    dispose(compute.pipeline);
}

void TessellationDemo::onSwapChainRecreation() {
    createRenderPipeline();
    createComputePipeline();
}

VkCommandBuffer *TessellationDemo::buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) {
    numCommandBuffers = 1;
    auto& commandBuffer = commandBuffers[imageIndex];

    VkCommandBufferBeginInfo beginInfo = initializers::commandBufferBeginInfo();
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    static std::array<VkClearValue, 2> clearValues;
    clearValues[0].color = {0, 0, 0, 1};
    clearValues[1].depthStencil = {1.0, 0u};

    VkRenderPassBeginInfo rPassInfo = initializers::renderPassBeginInfo();
    rPassInfo.clearValueCount = COUNT(clearValues);
    rPassInfo.pClearValues = clearValues.data();
    rPassInfo.framebuffer = framebuffers[imageIndex];
    rPassInfo.renderArea.offset = {0u, 0u};
    rPassInfo.renderArea.extent = swapChain.extent;
    rPassInfo.renderPass = renderPass;

    vkCmdBeginRenderPass(commandBuffer, &rPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    renderPatch(commandBuffer);

    renderUI(commandBuffer);
    vkCmdEndRenderPass(commandBuffer);

    vkEndCommandBuffer(commandBuffer);

    return &commandBuffer;
}

void TessellationDemo::renderPatch(VkCommandBuffer commandBuffer) {
    Pipeline* pipeline;
    Patch* patch;
    void* constants;
    uint32_t constantsSize;
    if(patchType == PATCH_TYPE_QUADS){
        switch(spacing){
            case SPACING_EQUAL:
                pipeline = &quad.equalSpacing;
                break;
            case SPACING_FRACTIONAL_EVEN:
                pipeline = &quad.fractionalEvenSpacing;
                break;
            case SPACING_FRACTIONAL_ODD:
                pipeline = &quad.fractionalOddSpacing;
                break;
        }
        patch = &quadPatch;
        constants = &quad.constants;
        constantsSize = sizeof(quad.constants);
    }else if(patchType == PATCH_TYPE_TRIANGLE){
        switch(spacing){
            case SPACING_EQUAL:
                pipeline = &tri.equalSpacing;
                break;
            case SPACING_FRACTIONAL_EVEN:
                pipeline = &tri.fractionalEvenSpacing;
                break;
            case SPACING_FRACTIONAL_ODD:
                pipeline = &tri.fractionalOddSpacing;
                break;
        }
        patch = &trianglePatch;
        constants = &tri.constants;
        constantsSize = sizeof(tri.constants);
    }else{
        switch(spacing){
            case SPACING_EQUAL:
                pipeline = &isoline.equalSpacing;
                break;
            case SPACING_FRACTIONAL_EVEN:
                pipeline = &isoline.fractionalEvenSpacing;
                break;
            case SPACING_FRACTIONAL_ODD:
                pipeline = &isoline.fractionalOddSpacing;
                break;
        }
        patch = &isolinePatch;
        constants = &isoline.constants;
        constantsSize = sizeof(isoline.constants);

    }
    render(commandBuffer, *pipeline, *patch, constants, constantsSize);
}

void
TessellationDemo::render(VkCommandBuffer commandBuffer, Pipeline &pipeline, Patch &patch, const void *constants, uint32_t constantsSize) {
    VkDeviceSize offset = 0;
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);
    vkCmdPushConstants(commandBuffer, pipeline.layout, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, 0, constantsSize, constants);
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, patch.vertices, &offset);
    vkCmdDraw(commandBuffer, 4, 1, 0, 0);
}

void TessellationDemo::update(float time) {
    VulkanBaseApp::update(time);
}

void TessellationDemo::checkAppInputs() {
    VulkanBaseApp::checkAppInputs();
}

void TessellationDemo::cleanup() {
    // TODO save pipeline cache
    VulkanBaseApp::cleanup();
}

void TessellationDemo::onPause() {
    VulkanBaseApp::onPause();
}

void TessellationDemo::renderUI(VkCommandBuffer commandBuffer) {
    ImGui::Begin("Tessellation");
    ImGui::SetWindowSize({0, 0});


    if(ImGui::CollapsingHeader("Type", ImGuiTreeNodeFlags_DefaultOpen)){
        ImGui::RadioButton("Quad", &patchType, PATCH_TYPE_QUADS); ImGui::SameLine();
        ImGui::RadioButton("Triangle", &patchType, PATCH_TYPE_TRIANGLE); ImGui::SameLine();
        ImGui::RadioButton("isolines", &patchType, PATCH_TYPE_ISOLINES);
    }
    if(ImGui::CollapsingHeader("Spacing", ImGuiTreeNodeFlags_DefaultOpen)){
        ImGui::RadioButton("Equal spacing", &spacing, SPACING_EQUAL);
        ImGui::RadioButton("Fractional even spacing", &spacing, SPACING_FRACTIONAL_EVEN);
        ImGui::RadioButton("Fractional odd spacing", &spacing, SPACING_FRACTIONAL_ODD);
    }

    static std::array<int, 4> outerLevels{{4, 4, 4, 4}};
    static std::array<int, 2> innerLevels{{4, 4}};
    if(ImGui::CollapsingHeader("Levels", ImGuiTreeNodeFlags_DefaultOpen)){
        if(patchType == PATCH_TYPE_QUADS){
            ImGui::SliderInt("outer 0", &outerLevels[0], 2, 64);
            ImGui::SliderInt("outer 1", &outerLevels[1], 2, 64);
            ImGui::SliderInt("outer 2", &outerLevels[2], 2, 64);
            ImGui::SliderInt("outer 3", &outerLevels[3], 2, 64);
            ImGui::SliderInt("inner 0", &innerLevels[0], 2, 64);
            ImGui::SliderInt("inner 1", &innerLevels[1], 2, 64);
        }else if (patchType == PATCH_TYPE_TRIANGLE){
            ImGui::SliderInt("outer 0", &outerLevels[0], 2, 64);
            ImGui::SliderInt("outer 1", &outerLevels[1], 2, 64);
            ImGui::SliderInt("outer 2", &outerLevels[2], 2, 64);
            ImGui::SliderInt("inner 0", &innerLevels[0], 2, 64);
        }else {
            ImGui::SliderInt("outer 0", &outerLevels[0], 1, 64);
            ImGui::SliderInt("outer 1", &outerLevels[1], 2, 64);
        }
    }
    if(patchType == PATCH_TYPE_QUADS){
        quad.constants.outerTessLevels[0] = outerLevels[0];
        quad.constants.outerTessLevels[1] = outerLevels[1];
        quad.constants.outerTessLevels[2] = outerLevels[2];
        quad.constants.outerTessLevels[3] = outerLevels[3];

        quad.constants.innerTessLevels[0] = innerLevels[0];
        quad.constants.innerTessLevels[1] = innerLevels[1];
    }else if(patchType == PATCH_TYPE_TRIANGLE){
        tri.constants.outerTessLevels[0] = outerLevels[0];
        tri.constants.outerTessLevels[1] = outerLevels[1];
        tri.constants.outerTessLevels[2] = outerLevels[2];

        tri.constants.innerTessLevel = innerLevels[0];
    }else if(patchType == PATCH_TYPE_ISOLINES){
        isoline.constants.outerTessLevels[0] = outerLevels[0];
        isoline.constants.outerTessLevels[1] = outerLevels[1];
    }
    ImGui::End();
    plugin(IM_GUI_PLUGIN).draw(commandBuffer);
}

void TessellationDemo::createPatches() {
    std::vector<glm::vec3> points;
    points.emplace_back(-0.5, -0.5, 0);
    points.emplace_back(0.5, -0.5, 0);
    points.emplace_back(0.5, 0.5, 0);
    points.emplace_back(-0.5, 0.5, 0);

    quadPatch.vertices = device.createDeviceLocalBuffer(points.data(), BYTE_SIZE(points), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    isolinePatch.vertices = device.createDeviceLocalBuffer(points.data(), BYTE_SIZE(points), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

    points = { {-0.5, -0.5, 0.0}, {0.5, -0.5, 0.0}, {0.0, 0.5, 0.0} };
    trianglePatch.vertices = device.createDeviceLocalBuffer(points.data(), BYTE_SIZE(points), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

void TessellationDemo::initCamera() {
    camera.model = glm::mat4(1);
    camera.view = glm::mat4(1);
    camera.proj = vkn::ortho(-1.f, 1.f, -1.f, 1.f, -1.f, 1.f);
}


int main(){
    try{

        Settings settings;
        settings.width = 1000;
        settings.height = 1000;
        settings.depthTest = true;
        settings.enabledFeatures.tessellationShader = VK_TRUE;
        settings.enabledFeatures.fillModeNonSolid = VK_TRUE;
        auto app = TessellationDemo{ settings };
        std::unique_ptr<Plugin> plugin = std::make_unique<ImGuiPlugin>();
        app.addPlugin(plugin);
        app.run();
    }catch(std::runtime_error& err){
        spdlog::error(err.what());
    }
}