#include "CurvesSurfaceDemo.hpp"

#include <memory>
#include "GraphicsPipelineBuilder.hpp"
#include "DescriptorSetBuilder.hpp"
#include "ImGuiPlugin.hpp"
#include "teapot.h"

CurvesSurfaceDemo::CurvesSurfaceDemo(const Settings& settings) : VulkanBaseApp("Curves and Surfaces", settings) {
    fileManager.addSearchPath(".");
    fileManager.addSearchPath("../../examples/curves_and_surfaces");
    fileManager.addSearchPath("../../examples/curves_and_surfaces/spv");
    fileManager.addSearchPath("../../examples/curves_and_surfaces/models");
    fileManager.addSearchPath("../../examples/curves_and_surfaces/patches");
    fileManager.addSearchPath("../../data/shaders");
    fileManager.addSearchPath("../../data/models");
    fileManager.addSearchPath("../../data/textures");
    fileManager.addSearchPath("../../data");
    movePointAction = &mapToMouse(0, "move_pint", Action::normal());
}

void CurvesSurfaceDemo::initApp() {
    createDescriptorPool();
    createCommandPool();
    createDescriptorSetLayouts();
    createPatches();
    initCameras();
    initUniforms();
    updateDescriptorSets();
    createPipelineCache();
    createRenderPipeline();
    createComputePipeline();
}

void CurvesSurfaceDemo::createDescriptorPool() {
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

void CurvesSurfaceDemo::createDescriptorSetLayouts() {
    mvp.layout = 
        device.descriptorSetLayoutBuilder()
            .binding(0)
                .descriptorType(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_GEOMETRY_BIT)
            .createLayout();

    globalUBo.layout =
        device.descriptorSetLayoutBuilder()
            .binding(0)
                .descriptorType(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_FRAGMENT_BIT)
            .createLayout();
    auto sets = descriptorPool.allocate({mvp.layout, globalUBo.layout});
    mvp.descriptorSet = sets[0];
    globalUBo.descriptorSet = sets[1];
    descriptorSets[0] = mvp.descriptorSet;
    descriptorSets[1] = globalUBo.descriptorSet;

}

void CurvesSurfaceDemo::updateDescriptorSets() {
    auto writes = initializers::writeDescriptorSets<2>();
    writes[0].dstSet = mvp.descriptorSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[0].descriptorCount = 1;
    VkDescriptorBufferInfo mvpBufferInfo{mvp.buffer, 0, VK_WHOLE_SIZE};
    writes[0].pBufferInfo = &mvpBufferInfo;

    writes[1].dstSet = globalUBo.descriptorSet;
    writes[1].dstBinding = 0;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[1].descriptorCount = 1;
    VkDescriptorBufferInfo gUboBufferInfo{globalUBo.buffer, 0, VK_WHOLE_SIZE};
    writes[1].pBufferInfo = &gUboBufferInfo;

    device.updateDescriptorSets(writes);
}

void CurvesSurfaceDemo::createCommandPool() {
    commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandBuffers = commandPool.allocateCommandBuffers(swapChainImageCount);
}

void CurvesSurfaceDemo::createPipelineCache() {
    pipelineCache = device.createPipelineCache();
}


void CurvesSurfaceDemo::createRenderPipeline() {
    auto builder = device.graphicsPipelineBuilder();
    pPoints.pipeline =
        builder
            .allowDerivatives()
            .shaderStage()
                .vertexShader(load("points.vert.spv"))
                .fragmentShader(load("points.frag.spv"))
            .vertexInputState()
                .addVertexBindingDescription(0, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX)
                .addVertexAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0)
            .inputAssemblyState()
                .points()
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
                    .polygonModeFill()
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
                .renderPass(renderPass)
                .layout()
                    .addPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Camera))
                .subpass(0)
                .name("points")
                .pipelineCache(pipelineCache)
            .build(pPoints.layout);

    pBezierCurve.pipeline =
        builder
            .basePipeline(pPoints.pipeline)
            .allowDerivatives()
            .shaderStage().clear()
                .vertexShader(load("patch.vert.spv"))
                .tessellationControlShader(load("bezier_curve.tesc.spv"))
                .tessellationEvaluationShader(load("bezier_curve.tese.spv"))
                .fragmentShader(load("patch.frag.spv"))
            .inputAssemblyState()
                .patches()
            .tessellationState()
                .patchControlPoints(4)
                .domainOrigin(VK_TESSELLATION_DOMAIN_ORIGIN_LOWER_LEFT)
            .rasterizationState()
                .polygonModeLine()
            .layout().clear()
                .addPushConstantRange(VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, 0, sizeof(Camera))
            .name("bezier_curve")
        .build(pBezierCurve.layout);

    pBezierSurface.pipeline =
        builder
            .basePipeline(pBezierCurve.pipeline)
            .shaderStage()
                .tessellationControlShader(load("bezier_surface.tesc.spv"))
                .tessellationEvaluationShader(load("bezier_surface2.tese.spv"))
                .geometryShader(load("wireframe.geom.spv"))
                .fragmentShader(load("wireframe.frag.spv"))
            .tessellationState()
                .patchControlPoints(16)
            .rasterizationState()
                .cullNone()
                .polygonModeFill()
            .layout().clear()
                .addPushConstantRange(TESSELLATION_SHADER_STAGES_ALL, 0, sizeof(pBezierSurface.constants))
                .addDescriptorSetLayout(mvp.layout)
                .addDescriptorSetLayout(globalUBo.layout)
            .name("bezier_surface")
        .build(pBezierSurface.layout);

    pSphereSurface.pipeline =
        builder
            .basePipeline(pBezierCurve.pipeline)
            .shaderStage()
                .tessellationControlShader(load("sphere_surface.tesc.spv"))
                .tessellationEvaluationShader(load("sphere_surface.tese.spv"))
            .tessellationState()
                .patchControlPoints(4)
            .rasterizationState()
            .layout().clear()
                .addPushConstantRange(TESSELLATION_SHADER_STAGES_ALL, 0, sizeof(pSphereSurface.constants))
                .addDescriptorSetLayout(mvp.layout)
                .addDescriptorSetLayout(globalUBo.layout)
            .name("sphere_surface")
        .build(pSphereSurface.layout);

    pCubeSurface.pipeline =
        builder
            .basePipeline(pBezierCurve.pipeline)
            .shaderStage()
                .tessellationControlShader(load("cube.tesc.spv"))
                .tessellationEvaluationShader(load("cube.tese.spv"))
            .layout().clear()
                .addPushConstantRange(TESSELLATION_SHADER_STAGES_ALL, 0, sizeof(pCubeSurface.constants))
                .addDescriptorSetLayout(mvp.layout)
                .addDescriptorSetLayout(globalUBo.layout)
            .name("cube_surface")
        .build(pCubeSurface.layout);

    pIcoSphereSurface.pipeline =
        builder
            .basePipeline(pBezierSurface.pipeline)
            .shaderStage()
                .tessellationControlShader(load("ico_sphere_surface.tesc.spv"))
                .tessellationEvaluationShader(load("ico_sphere_surface.tese.spv"))
            .tessellationState()
                .patchControlPoints(3)
            .layout().clear()
                .addPushConstantRange(TESSELLATION_SHADER_STAGES_ALL, 0, sizeof(pIcoSphereSurface.constants))
                .addDescriptorSetLayout(mvp.layout)
                .addDescriptorSetLayout(globalUBo.layout)
            .name("iso_sphere_surface")
        .build(pIcoSphereSurface.layout);
}

void CurvesSurfaceDemo::createComputePipeline() {
    auto module = VulkanShaderModule{ "../../data/shaders/pass_through.comp.spv", device};
    auto stage = initializers::shaderStage({ module, VK_SHADER_STAGE_COMPUTE_BIT});

    compute.layout = device.createPipelineLayout();

    auto computeCreateInfo = initializers::computePipelineCreateInfo();
    computeCreateInfo.stage = stage;
    computeCreateInfo.layout = compute.layout;

    compute.pipeline = device.createComputePipeline(computeCreateInfo, pipelineCache);
}


void CurvesSurfaceDemo::onSwapChainDispose() {
    dispose(pBezierCurve.pipeline);
    dispose(compute.pipeline);
}

void CurvesSurfaceDemo::onSwapChainRecreation() {
    createRenderPipeline();
    createComputePipeline();
}

VkCommandBuffer *CurvesSurfaceDemo::buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) {
    numCommandBuffers = 1;
    auto& commandBuffer = commandBuffers[imageIndex];

    VkCommandBufferBeginInfo beginInfo = initializers::commandBufferBeginInfo();
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    static std::array<VkClearValue, 2> clearValues;
    if(mode == MODE_CURVES){
        clearValues[0].color = {0, 0, 0, 1};
    }else {
        clearValues[0].color = {0.8, 0.8, 0.8, 1};
    }
    clearValues[1].depthStencil = {1.0, 0u};

    VkRenderPassBeginInfo rPassInfo = initializers::renderPassBeginInfo();
    rPassInfo.clearValueCount = COUNT(clearValues);
    rPassInfo.pClearValues = clearValues.data();
    rPassInfo.framebuffer = framebuffers[imageIndex];
    rPassInfo.renderArea.offset = {0u, 0u};
    rPassInfo.renderArea.extent = swapChain.extent;
    rPassInfo.renderPass = renderPass;

    vkCmdBeginRenderPass(commandBuffer, &rPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    if(mode == MODE_CURVES) {
        if(curve == CURVE_BEZIER) {
            renderBezierCurve(commandBuffer);
            renderPoints(commandBuffer);
        }
    }else if(surface == SURFACE_SPHERE){
        renderSphereSurface(commandBuffer);
    }else if(surface == SURFACE_CUBE){
        renderCubeSurface(commandBuffer);
    }else if(surface == SURFACE_ICO_SPHERE){
        renderIcoSphereSurface(commandBuffer);
    }
    else{
        renderBezierSurface(commandBuffer);
    }
    renderUI(commandBuffer);
    vkCmdEndRenderPass(commandBuffer);
    vkEndCommandBuffer(commandBuffer);

    return &commandBuffer;
}

void CurvesSurfaceDemo::renderUI(VkCommandBuffer commandBuffer) {
    ImGui::Begin("Curves & Surfaces");
    ImGui::SetWindowSize({0, 0});

    if(ImGui::CollapsingHeader("Mode", ImGuiTreeNodeFlags_DefaultOpen)){
        ImGui::RadioButton("Curves", &mode, MODE_CURVES);
        ImGui::RadioButton("Surfaces", &mode, MODE_SURFACE);
    }
    if(mode == MODE_CURVES && ImGui::CollapsingHeader("Curves", ImGuiTreeNodeFlags_DefaultOpen)){
        ImGui::RadioButton("Bezier", &curve, CURVE_BEZIER);
    }
    if(mode == MODE_SURFACE && ImGui::CollapsingHeader("Surfaces", ImGuiTreeNodeFlags_DefaultOpen)){
        static const char* primitives[6]{"Teapot", "Teacup", "Teaspoon", "Sphere", "IcoSphere", "Cube"};

        static int primitive = 0;
        ImGui::Combo("Primitive", &primitive, primitives, 6);
        surface = (1 << primitive);

        if(surface & SURFACE_BEZIER){
            auto& constants = pBezierSurface.constants;
            tessellationLevelUI(constants.tessLevelOuter, constants.tessLevelInner, constants.u, constants.v);
        }else if(surface == SURFACE_SPHERE){
            auto& constants = pSphereSurface.constants;
            tessellationLevelUI(constants.tessLevelOuter, constants.tessLevelInner, constants.u, constants.v);
            ImGui::SliderFloat("radius", &constants.radius, 1.0, 5.0);
        }else if(surface == SURFACE_CUBE){
            auto& constants = pCubeSurface.constants;
            tessellationLevelUI(constants.tessLevelOuter, constants.tessLevelInner, constants.u, constants.v);
            ImGui::SliderFloat3("scale", constants.scale, 0.5, 5);
            static bool normalize = constants.normalize;
            ImGui::Checkbox("normalize", &normalize);
            constants.normalize = normalize;
        }else if(surface == SURFACE_ICO_SPHERE){
            auto& constants = pIcoSphereSurface.constants;
            tessellationLevelUI(constants.tessLevelOuter, constants.tessLevelInner, constants.u, constants.v);
            ImGui::SliderFloat("w", &constants.w, 0.0, 1.0);
            ImGui::SliderFloat("radius", &constants.radius, 1.0, 5.0);
        }
        ImGui::PushID("prim_color");
        ImGui::ColorEdit3("color", reinterpret_cast<float*>(&globalConstants.surfaceColor));
        ImGui::PopID();
    }

    if(ImGui::CollapsingHeader("Global", ImGuiTreeNodeFlags_DefaultOpen)){
        static bool wireframeEnabled = globalConstants.wireframe.enabled;
        static bool solid = globalConstants.wireframe.solid;
        ImGui::Checkbox("wireframe", &wireframeEnabled); ImGui::SameLine();
        if(wireframeEnabled && mode == MODE_SURFACE){
            ImGui::Checkbox("solid", &solid);
            ImGui::ColorEdit3("color", reinterpret_cast<float*>(&globalConstants.wireframe.color));
            ImGui::SliderFloat("width", &globalConstants.wireframe.width, 0.01, 0.1);
        }
        globalConstants.wireframe.enabled = wireframeEnabled;
        globalConstants.wireframe.solid = solid;
    }

    ImGui::End();

    plugin(IM_GUI_PLUGIN).draw(commandBuffer);
}

void CurvesSurfaceDemo::tessellationLevelUI(float &outer, float &inner, float& u, float& v) {
    static bool link = true;
    auto outerUpdated = ImGui::SliderFloat("Outer tess levels", &outer, 1, 64); ImGui::SameLine();
    ImGui::Checkbox("link", &link);
    auto innerUpdated = ImGui::SliderFloat("Inner tess levels", &inner, 1, 64);
    ImGui::SliderFloat("u", &u, 0, 1);
    ImGui::SliderFloat("v", &v, 0, 1);

    if(link){
        if(outerUpdated) inner = outer;
        if(innerUpdated) outer = inner;
    }
}

void CurvesSurfaceDemo::renderBezierCurve(VkCommandBuffer commandBuffer) {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pBezierCurve.pipeline);
    vkCmdPushConstants(commandBuffer, pBezierCurve.layout, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, 0, sizeof(Camera), &camera2d);
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, bezierCurve.points, &offset);
    vkCmdBindIndexBuffer(commandBuffer, bezierCurve.indices, 0, VK_INDEX_TYPE_UINT16);
    vkCmdDrawIndexed(commandBuffer, 4, 1, 0, 0, 0);
}

void CurvesSurfaceDemo::renderBezierSurface(VkCommandBuffer commandBuffer){
    if(surface == SURFACE_TEAPOT) {
        renderBezierSurface(commandBuffer, teapot);
    }
    switch(surface){
        case SURFACE_TEAPOT :
            renderBezierSurface(commandBuffer, teapot);
            break;
        case SURFACE_TEACUP:
            renderBezierSurface(commandBuffer, teacup);
            break;
        case SURFACE_TEASPOON:
            renderBezierSurface(commandBuffer, teaspoon);
        case SURFACE_SPHERE:
            renderBezierSurface(commandBuffer, sphere);
    }
}


void CurvesSurfaceDemo::renderBezierSurface(VkCommandBuffer commandBuffer, Patch &patch) {
    VkDeviceSize offset = 0;
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pBezierSurface.pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pBezierSurface.layout, 0, COUNT(descriptorSets), descriptorSets.data(), 0, VK_NULL_HANDLE);
    vkCmdPushConstants(commandBuffer, pBezierSurface.layout, TESSELLATION_SHADER_STAGES_ALL, 0, sizeof(pBezierSurface.constants), &pBezierSurface.constants);
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, patch.points, &offset);
    vkCmdBindIndexBuffer(commandBuffer, patch.indices, 0, VK_INDEX_TYPE_UINT16);
    vkCmdDrawIndexed(commandBuffer, patch.indices.size/sizeof(int16_t), 1, 0, 0, 0);
}

void CurvesSurfaceDemo::renderSphereSurface(VkCommandBuffer commandBuffer) {
    VkDeviceSize offset = 0;
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pSphereSurface.pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pSphereSurface.layout, 0, COUNT(descriptorSets), descriptorSets.data(), 0, VK_NULL_HANDLE);
    vkCmdPushConstants(commandBuffer, pSphereSurface.layout, TESSELLATION_SHADER_STAGES_ALL, 0, sizeof(pSphereSurface.constants), &pSphereSurface.constants);
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, sphere.points, &offset);
    vkCmdBindIndexBuffer(commandBuffer, sphere.indices, 0, VK_INDEX_TYPE_UINT16);
    vkCmdDrawIndexed(commandBuffer, sphere.indices.size/sizeof(int16_t), 1, 0, 0, 0);
}

void CurvesSurfaceDemo::renderIcoSphereSurface(VkCommandBuffer commandBuffer) {
    VkDeviceSize offset = 0;
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pIcoSphereSurface.pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pIcoSphereSurface.layout, 0, COUNT(descriptorSets), descriptorSets.data(), 0, VK_NULL_HANDLE);
    vkCmdPushConstants(commandBuffer, pIcoSphereSurface.layout, TESSELLATION_SHADER_STAGES_ALL, 0, sizeof(pIcoSphereSurface.constants), &pIcoSphereSurface.constants);
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, icoSphere.points, &offset);
    vkCmdBindIndexBuffer(commandBuffer, icoSphere.indices, 0, VK_INDEX_TYPE_UINT16);
    vkCmdDrawIndexed(commandBuffer, icoSphere.indices.size/sizeof(int16_t), 1, 0, 0, 0);
}

void CurvesSurfaceDemo::renderCubeSurface(VkCommandBuffer commandBuffer) {
    VkDeviceSize offset = 0;
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pCubeSurface.pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pCubeSurface.layout, 0, COUNT(descriptorSets), descriptorSets.data(), 0, VK_NULL_HANDLE);
    vkCmdPushConstants(commandBuffer, pCubeSurface.layout, TESSELLATION_SHADER_STAGES_ALL, 0, sizeof(pCubeSurface.constants), &pCubeSurface.constants);
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, cube.points, &offset);
    vkCmdBindIndexBuffer(commandBuffer, cube.indices, 0, VK_INDEX_TYPE_UINT16);
    vkCmdDrawIndexed(commandBuffer, cube.indices.size/sizeof(int16_t), 1, 0, 0, 0);
}

void CurvesSurfaceDemo::renderPoints(VkCommandBuffer commandBuffer)  {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pPoints.pipeline);
    vkCmdPushConstants(commandBuffer, pPoints.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Camera), &camera2d);
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, bezierCurve.points, &offset);
    vkCmdBindIndexBuffer(commandBuffer, bezierCurve.indices, 0, VK_INDEX_TYPE_UINT16);
    vkCmdDrawIndexed(commandBuffer, 4, 1, 0, 0, 0);
}


void CurvesSurfaceDemo::update(float time) {
    if(!ImGui::IsAnyItemActive()){
        cameraController->update(time);
        mvp.buffer.copy(&cameraController->cam(), sizeof(Camera));
        globalUBo.buffer.copy(&globalConstants, sizeof(globalConstants));
    }
}

void CurvesSurfaceDemo::checkAppInputs() {
    cameraController->processInput();

    if(mode == MODE_CURVES && curve == CURVE_BEZIER) {
        if (movePointAction->isPressed() && selectedPoint == -1) {
            auto point = selectPoint();
            selectedPoint = isPointOnScreen(point);
        }
        if (movePointAction->isPressed() && selectedPoint != -1) {
            auto position = selectPoint();
            movePoint(selectedPoint, position);
        } else {
            selectedPoint = -1;
        }
    }

}

void CurvesSurfaceDemo::cleanup() {
    bezierCurve.points.unmap();
}

void CurvesSurfaceDemo::onPause() {
    VulkanBaseApp::onPause();
}


void CurvesSurfaceDemo::movePoint(int pointIndex, glm::vec3 position) {
    assert(pointsHandle);
    pointsHandle[pointIndex] = position;
}

glm::vec3 CurvesSurfaceDemo::selectPoint() {
    auto model = camera2d.view * camera2d.model;
    auto projection = camera2d.proj;
    auto viewport = glm::vec4{0, 0, width, height};
    auto windowPoint = glm::vec3(mouse.position, 0);
    auto point = glm::unProject(windowPoint, model, projection, viewport);
    point.z = 0;
    return point;
}

void CurvesSurfaceDemo::initCameras() {
    OrbitingCameraSettings settings{};
    settings.offsetDistance = 10.0f;
    settings.orbitMinZoom = 0.1f;
    settings.orbitMaxZoom = 10.f;
    settings.rotationSpeed = 0.1f;
    settings.zNear = 0.1f;
    settings.zFar = 20.0f;
    settings.fieldOfView = 45.0f;
    settings.modelHeight = 0;
    settings.aspectRatio = static_cast<float>(swapChain.extent.width)/static_cast<float>(swapChain.extent.height);
    cameraController = std::make_unique<OrbitingCameraController>( device, swapChain.imageCount(), currentImageIndex,
                                                                   static_cast<InputManager&>(*this), settings);
    camera2d.proj = vkn::ortho(-1, 1, -1, 1, -1, 1);

}

void CurvesSurfaceDemo::initUniforms() {
    mvp.buffer = device.createCpuVisibleBuffer(&cameraController->cam(), sizeof(Camera), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    globalUBo.buffer = device.createCpuVisibleBuffer(&globalConstants, sizeof(globalConstants), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
}

void CurvesSurfaceDemo::createBezierCurvePatch() {
    std::vector<glm::vec3> points{
            {-0.5, -0.5, 0.0},
            {-0.3, 0.5, 0.0},
            {0.3, 0.5, 0.0},
            { 0.5, -0.5, 0.0}
    };
    std::vector<uint16_t> indices{ 0, 1, 2, 3};
    bezierCurve.points = device.createCpuVisibleBuffer(points.data(), BYTE_SIZE(points), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    bezierCurve.indices = device.createCpuVisibleBuffer(indices.data(), BYTE_SIZE(indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    pointsHandle = reinterpret_cast<glm::vec3*>(bezierCurve.points.map());
}

int CurvesSurfaceDemo::isPointOnScreen(glm::vec3 aPoint) {
    int index = -1;
    auto points = pointsHandle;
    float minDist = std::numeric_limits<float>::max();
    for(int i = 0; i < numPoints; i++){
        auto point = points[i];
        auto dist = glm::distance(aPoint, point);
        if(dist < minDist){
            index = i;
            minDist = dist;
        }
    }
    return  index != -1 && minDist < 0.01 ? index : -1;
}

void CurvesSurfaceDemo::createPatches() {
    createSpherePatch();
    createCubePatch();
    createBezierCurvePatch();
    loadBezierPatches();
    createIcoSpherePatch();
}

void CurvesSurfaceDemo::loadBezierPatches() {
//    loadPatch("teapot", teapot);
    auto [nodes, indices] = teapotPatch();
    teapot.points = device.createDeviceLocalBuffer(nodes.data(), BYTE_SIZE(nodes), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    teapot.indices = device.createDeviceLocalBuffer(indices.data(), BYTE_SIZE(indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
//    loadPatch("teacup", teacup);
    loadTeaCup();
    loadPatch("teaspoon", teaspoon);
}

void CurvesSurfaceDemo::loadPatch(const std::string &name, Patch &patch) {
    spdlog::info("loading patch: {}", name);
    auto nodePath = resource(fmt::format("{}_nodes.txt", name));
    std::ifstream fin(nodePath.data());
    if(!fin.good()) throw  std::runtime_error{fmt::format("unable to open {}}", nodePath)};

    std::vector<glm::vec3> nodes;
    while(!fin.eof()){
        glm::vec3 node{0};
        fin >> node.x >> node.y >> node.z;
        nodes.push_back(node);
    }
    fin.close();

    auto rectanglesPath = resource(fmt::format("{}_rectangles.txt", name));
    fin = std::ifstream(rectanglesPath);
    if(!fin.good()) throw  std::runtime_error{fmt::format("unable to open {}}", rectanglesPath)};

    std::vector<uint16_t> rectangles;
    while(!fin.eof()){
        uint16_t index;
        fin >> index;
        rectangles.push_back(index);
    }
    patch.points = device.createCpuVisibleBuffer(nodes.data(), BYTE_SIZE(nodes), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    patch.indices = device.createDeviceLocalBuffer(rectangles.data(), BYTE_SIZE(rectangles), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
}

void CurvesSurfaceDemo::loadTeaCup() {
    std::string name = "teacup";
    spdlog::info("loading teacup");
    auto nodePath = resource(fmt::format("{}.txt", name));
    std::ifstream fin(nodePath.data());
    if(!fin.good()) throw  std::runtime_error{fmt::format("unable to open {}}", nodePath)};

    std::vector<glm::vec3> nodes;
    while(!fin.eof()){
        glm::vec3 node{0};
        fin >> node.x >> node.y >> node.z;
        nodes.push_back(node);
    }
    fin.close();

    teacup.points = device.createCpuVisibleBuffer(nodes.data(), BYTE_SIZE(nodes), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

    std::vector<uint16_t> rectangles(nodes.size());
    std::iota(begin(rectangles), end(rectangles), 0);
    teacup.indices = device.createDeviceLocalBuffer(rectangles.data(), BYTE_SIZE(rectangles), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
}

void CurvesSurfaceDemo::createSpherePatch() {
    std::vector<glm::vec3> points(4, glm::vec3(0));
    std::vector<uint16_t> indices{0, 1, 2, 3};
    sphere.points = device.createCpuVisibleBuffer(points.data(), BYTE_SIZE(points), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    sphere.indices = device.createCpuVisibleBuffer(indices.data(), BYTE_SIZE(indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
}

void CurvesSurfaceDemo::createIcoSpherePatch() {
    std::vector<glm::vec3> points{
            {0.000f,  0.000f,  1.000f},
            {0.894f,  0.000f,  0.447f},
            {0.276f,  0.851f,  0.447f},
            {-0.724f,  0.526f,  0.447f},
            {-0.724f, -0.526f,  0.447f},
            {0.276f, -0.851f,  0.447f},
            {0.724f,  0.526f, -0.447f},
            {-0.276f,  0.851f, -0.447f},
            {-0.894f,  0.000f, -0.447f},
            {-0.276f, -0.851f, -0.447f},
            {0.724f, -0.526f, -0.447f},
            {0.000f,  0.000f, -1.000f}
    };

    std::vector<uint16_t> indices{
            2, 1, 0,
            3, 2, 0,
            4, 3, 0,
            5, 4, 0,
            1, 5, 0,
            11, 6,  7,
            11, 7,  8,
            11, 8,  9,
            11, 9,  10,
            11, 10, 6,
            1, 2, 6,
            2, 3, 7,
            3, 4, 8,
            4, 5, 9,
            5, 1, 10,
            2,  7, 6,
            3,  8, 7,
            4,  9, 8,
            5, 10, 9,
            1, 6, 10
    };
    icoSphere.points = device.createCpuVisibleBuffer(points.data(), BYTE_SIZE(points), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    icoSphere.indices = device.createCpuVisibleBuffer(indices.data(), BYTE_SIZE(indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
}

void CurvesSurfaceDemo::createCubePatch() {
    std::vector<glm::vec3> points{
            {-.5, -.5,  .5}, {.5, -.5,  .5},  {.5, .5,  .5}, {-.5, .5,  .5},
            {-.5, -.5, -.5}, {.5, -.5, -.5},  {.5, .5, -.5}, {-.5, .5, -.5}
    };

    std::vector<uint16_t> indices{
        0, 1, 2, 3, // front
        1, 5, 6, 2, // right
        5, 4, 7, 6, // back
        4, 0, 3, 7, // left
        3, 2, 6, 7, // top
        4, 5, 1, 0 //  bottom
    };

    cube.points = device.createCpuVisibleBuffer(points.data(), BYTE_SIZE(points), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    cube.indices = device.createCpuVisibleBuffer(indices.data(), BYTE_SIZE(indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
}

int main(){
    try{

        Settings settings;
        settings.depthTest = true;
        settings.enabledFeatures.fillModeNonSolid = VK_TRUE;
        settings.enabledFeatures.tessellationShader = VK_TRUE;
        settings.enabledFeatures.largePoints = VK_TRUE;
        settings.enabledFeatures.geometryShader = VK_TRUE;
        auto app = CurvesSurfaceDemo{ settings };
        std::unique_ptr<Plugin> plugin = std::make_unique<ImGuiPlugin>();
        app.addPlugin(plugin);
        app.run();
    }catch(std::runtime_error& err){
        spdlog::error(err.what());
    }
}