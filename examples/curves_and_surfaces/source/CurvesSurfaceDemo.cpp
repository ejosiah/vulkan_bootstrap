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
    fileManager.addSearchPath("../../examples/curves_and_surfaces/textures");
    fileManager.addSearchPath("../../data/shaders");
    fileManager.addSearchPath("../../data/models");
    fileManager.addSearchPath("../../data/textures");
    fileManager.addSearchPath("../../data");
    movePointAction = &mapToMouse(0, "move_point", Action::normal());
    addPointAction = &mapToMouse(0, "add_point", Action::detectInitialPressOnly());
}

void CurvesSurfaceDemo::initApp() {
    createDescriptorPool();
    createCommandPool();
    createDescriptorSetLayouts();
    createPatches();
    initCameras();
    initUniforms();
    loadTexture();
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
                .shaderStages(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_GEOMETRY_BIT)
            .createLayout();

    globalUBo.layout =
        device.descriptorSetLayoutBuilder()
            .binding(0)
                .descriptorType(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_FRAGMENT_BIT)
            .createLayout();
    
    vulkanImage.layout =
        device.descriptorSetLayoutBuilder()
            .binding(0)
                .descriptorType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_FRAGMENT_BIT)
            .createLayout();

    profileCurve.layout =
        device.descriptorSetLayoutBuilder()
            .binding(0)
                .descriptorType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)
            .binding(1)
                .descriptorType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)
            .createLayout();

    createDescriptorSets();

}

void CurvesSurfaceDemo::createDescriptorSets() {
    auto sets = descriptorPool.allocate({mvp.layout, globalUBo.layout, vulkanImage.layout, profileCurve.layout});
    mvp.descriptorSet = sets[0];
    globalUBo.descriptorSet = sets[1];
    vulkanImage.descriptorSet = sets[2];
    profileCurve.descriptorSet = sets[3];
    descriptorSets[0] = mvp.descriptorSet;
    descriptorSets[1] = globalUBo.descriptorSet;
    descriptorSets[2] = vulkanImage.descriptorSet;
}

void CurvesSurfaceDemo::refreshDescriptorSets() {
    createDescriptorSets();
    updateDescriptorSets();
}

void CurvesSurfaceDemo::updateDescriptorSets() {
    auto writes = initializers::writeDescriptorSets<5>();
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

    writes[2].dstSet = vulkanImage.descriptorSet;
    writes[2].dstBinding = 0;
    writes[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[2].descriptorCount = 1;
    VkDescriptorImageInfo vulkanImageInfo{vulkanImage.texture.sampler, vulkanImage.texture.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    writes[2].pImageInfo = &vulkanImageInfo;
    
    writes[3].dstSet = profileCurve.descriptorSet;
    writes[3].dstBinding = 0;
    writes[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[3].descriptorCount = 1;
    VkDescriptorBufferInfo profileVertexInfo{profileCurve.vertices, 0, VK_WHOLE_SIZE};
    writes[3].pBufferInfo = &profileVertexInfo;

    writes[4].dstSet = profileCurve.descriptorSet;
    writes[4].dstBinding = 1;
    writes[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[4].descriptorCount = 1;
    VkDescriptorBufferInfo profileIndexInfo{profileCurve.indices, 0, VK_WHOLE_SIZE};
    writes[4].pBufferInfo = &profileIndexInfo;

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
//    @formatter:off
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
                    .addDescriptorSetLayout(mvp.layout)
                .subpass(0)
                .name("points")
                .pipelineCache(pipelineCache)
            .build(pPoints.layout);

    pTangentLines.pipeline =
        builder
            .basePipeline(pPoints.pipeline)
            .inputAssemblyState()
                .lines()
            .name("tangent_lines")
        .build(pTangentLines.layout);

    pBezierCurve.pipeline =
        builder
            .basePipeline(pPoints.pipeline)
            .allowDerivatives()
            .shaderStage().clear()
                .vertexShader(load("patch.vert.spv"))
                .tessellationControlShader(load("curve.tesc.spv"))
                .tessellationEvaluationShader(load("bezier_curve.tese.spv"))
                .fragmentShader(load("patch.frag.spv"))
            .inputAssemblyState()
                .patches()
            .tessellationState()
                .patchControlPoints(4)
                .domainOrigin(VK_TESSELLATION_DOMAIN_ORIGIN_LOWER_LEFT)
            .rasterizationState()
                .polygonModeFill()
            .layout()
                .addPushConstantRange(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, 0, sizeof(pBezierCurve.constants))
            .name("bezier_curve")
        .build(pBezierCurve.layout);
		
	pHermiteCurve.pipeline =
		builder
			.basePipeline(pBezierCurve.pipeline)
			.shaderStage()
				.tessellationEvaluationShader(load("hermite_curve.tese.spv"))
			.layout().clear()
				.addDescriptorSetLayout(mvp.layout)
				.addPushConstantRange(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, 0, sizeof(pHermiteCurve.constants))
			.name("hermite_curve")
		.build(pHermiteCurve.layout);

    pArcCurve.pipeline =
        builder
            .basePipeline(pBezierCurve.pipeline)
            .shaderStage()
                .tessellationEvaluationShader(load("arc.tese.spv"))
            .tessellationState()
                .patchControlPoints(2)
            .layout().clear()
                .addDescriptorSetLayout(mvp.layout)
                .addPushConstantRange(TESSELLATION_SHADER_STAGES_ALL, 0, sizeof(pArcCurve.constants))
            .name("arc")
        .build(pArcCurve.layout);

    pBezierSurface.pipeline =
        builder
            .basePipeline(pBezierCurve.pipeline)
            .shaderStage()
                .tessellationControlShader(load("bezier_surface.tesc.spv"))
                .tessellationEvaluationShader(load("bezier_surface.tese.spv"))
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
                .addDescriptorSetLayout(vulkanImage.layout)
                .name("bezier_surface")
        .build(pBezierSurface.layout);

    pSphereSurface.pipeline =
        builder
            .basePipeline(pBezierCurve.pipeline)
            .allowDerivatives()
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
                .addDescriptorSetLayout(vulkanImage.layout)
                .name("sphere_surface")
        .build(pSphereSurface.layout);

    pSurfaceRevolution.pipeline =
        builder
            .basePipeline(pBezierCurve.pipeline)
            .allowDerivatives()
            .shaderStage()
                .tessellationControlShader(load("revolution.tesc.spv"))
                .tessellationEvaluationShader(load("revolution.tese.spv"))
            .tessellationState()
                .patchControlPoints(4)
            .layout().clear()
                .addPushConstantRange(TESSELLATION_SHADER_STAGES_ALL, 0, sizeof(pSurfaceRevolution.constants))
                .addDescriptorSetLayout(mvp.layout)
                .addDescriptorSetLayout(globalUBo.layout)
                .addDescriptorSetLayout(vulkanImage.layout)
                .addDescriptorSetLayout(profileCurve.layout)
                .name("surface_revolution")
        .build(pSurfaceRevolution.layout);
		
	pTorusSurface.pipeline =
        builder
            .basePipeline(pBezierCurve.pipeline)
            .shaderStage()
                .tessellationControlShader(load("torus.tesc.spv"))
                .tessellationEvaluationShader(load("torus.tese.spv"))
            .rasterizationState()
                .polygonModeFill()
            .layout().clear()
                .addPushConstantRange(TESSELLATION_SHADER_STAGES_ALL, 0, sizeof(pTorusSurface.constants))
                .addDescriptorSetLayout(mvp.layout)
                .addDescriptorSetLayout(globalUBo.layout)
                .addDescriptorSetLayout(vulkanImage.layout)
            .name("Torus_surface")
        .build(pTorusSurface.layout);

	pCylinder.pipeline =
        builder
            .basePipeline(pBezierCurve.pipeline)
            .shaderStage()
                .tessellationControlShader(load("cylinder.tesc.spv"))
                .tessellationEvaluationShader(load("cylinder.tese.spv"))
            .layout().clear()
                .addPushConstantRange(TESSELLATION_SHADER_STAGES_ALL, 0, sizeof(pCylinder.constants))
                .addDescriptorSetLayout(mvp.layout)
                .addDescriptorSetLayout(globalUBo.layout)
                .addDescriptorSetLayout(vulkanImage.layout)
            .name("Cylinder_surface")
        .build(pCylinder.layout);

	pCone.pipeline =
        builder
            .basePipeline(pBezierCurve.pipeline)
            .shaderStage()
                .tessellationControlShader(load("cylinder.tesc.spv"))
                .tessellationEvaluationShader(load("cone.tese.spv"))
            .layout().clear()
                .addPushConstantRange(TESSELLATION_SHADER_STAGES_ALL, 0, sizeof(pCone.constants))
                .addDescriptorSetLayout(mvp.layout)
                .addDescriptorSetLayout(globalUBo.layout)
                .addDescriptorSetLayout(vulkanImage.layout)
            .name("Cone_surface")
        .build(pCone.layout);

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
                .addDescriptorSetLayout(vulkanImage.layout)
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
                .addDescriptorSetLayout(vulkanImage.layout)
                .name("iso_sphere_surface")
        .build(pIcoSphereSurface.layout);

    pSphereSurface.xray.pipeline =
        builder.basePipeline(pSphereSurface.pipeline)
            .shaderStage().clear()
                .vertexShader(load("patch.vert.spv"))
                .tessellationControlShader(load("curve.tesc.spv"))
                .tessellationEvaluationShader(load("sphere_xray.tese.spv"))
                .fragmentShader(load("patch.frag.spv"))
            .tessellationState()
                .patchControlPoints(4)
            .layout().clear()
                .addDescriptorSetLayout(mvp.layout)
                .addPushConstantRange(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, 0, sizeof(std::array<int, 2>))
                .addPushConstantRange(VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, sizeof(std::array<int, 2>), sizeof(pSphereSurface.constants))
            .name("sphere_xray")
        .build(pSphereSurface.xray.layout);
//    @formatter:on
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
    updateDescriptorSets();
    updateCameras();
    createRenderPipeline();
    createComputePipeline();
}

VkCommandBuffer *CurvesSurfaceDemo::buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) {
    numCommandBuffers = 1;
    auto& commandBuffer = commandBuffers[imageIndex];

    VkCommandBufferBeginInfo beginInfo = initializers::commandBufferBeginInfo();
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    static std::array<VkClearValue, 2> clearValues;
    if(mode == MODE_CURVES || (mode == MODE_SURFACE && surface == SURFACE_SPHERE && pSphereSurface.xray.enabled)){
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
        renderCurve(commandBuffer);
    }else if(surface == SURFACE_SPHERE){
        renderSphereSurface(commandBuffer);
    }else if(surface == SURFACE_CUBE){
        renderCubeSurface(commandBuffer);
    }else if(surface == SURFACE_ICO_SPHERE){
        renderIcoSphereSurface(commandBuffer);
    }else if (surface == SURFACE_TORUS){
        renderTorusSurface(commandBuffer);
    }else if(surface == SURFACE_CYLINDER){
        renderCylinderSurface(commandBuffer);
    }else if(surface == SURFACE_CONE){
        renderCone(commandBuffer);
    }else if(surface == SURFACE_SWEPT){
        renderSweptSurface(commandBuffer);
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
        ImGui::RadioButton("Curves", &mode, MODE_CURVES); ImGui::SameLine();
        ImGui::RadioButton("Surfaces", &mode, MODE_SURFACE);
    }
    if(mode == MODE_CURVES && ImGui::CollapsingHeader("Curves", ImGuiTreeNodeFlags_DefaultOpen)){
        static int aCurve = log2(curve);
        static const char* curves[3]{"Bezier", "Hermite", "Arc"};
        ImGui::Combo("Curve", &aCurve, curves, std::size(curves));
        curve = (1 << aCurve);
        if(curve == CURVE_ARC){
            static float rad = glm::radians(pArcCurve.constants.angle);
            static bool clockwise = static_cast<bool>(pArcCurve.constants.clockwise);
            ImGui::SliderAngle("angle", &rad, 0);
            ImGui::SliderFloat("radius", &pArcCurve.constants.radius, 0.1, 1);
            ImGui::Checkbox("Clockwise", &clockwise);
            pArcCurve.constants.angle = glm::degrees(rad);
            pArcCurve.constants.clockwise = static_cast<int>(clockwise);
        }
        ImGui::SliderInt("resolution", &curveResolution, 4, 1000);

    }
    if(mode == MODE_SURFACE && ImGui::CollapsingHeader("Surfaces", ImGuiTreeNodeFlags_DefaultOpen)){
        static const char* primitives[10]{"Teapot", "Teacup", "Teaspoon", "Sphere", "IcoSphere", "Cube", "Torus", "Cylinder", "Cone", "Surface revolution"};

        static int primitive = log2(surface);
        ImGui::Combo("Primitive", &primitive, primitives, std::size(primitives));
        surface = (1 << primitive);

        if(surface & SURFACE_BEZIER){
            auto& constants = pBezierSurface.constants;
            tessellationLevelUI(constants.tessLevelOuter, constants.tessLevelInner, constants.u, constants.v);
        }else if(surface == SURFACE_SPHERE){
            auto& constants = pSphereSurface.constants;
            ImGui::SameLine(); ImGui::Checkbox("xray", &pSphereSurface.xray.enabled);
            tessellationLevelUI(constants.tessLevelOuter, constants.tessLevelInner, constants.u, constants.v);
            ImGui::SliderFloat("radius", &constants.radius, 1.0, 5.0);
        }else if(surface == SURFACE_TORUS){
            auto& constants = pTorusSurface.constants;
            tessellationLevelUI(constants.tessLevelOuter, constants.tessLevelInner, constants.u, constants.v);
            ImGui::SliderFloat("radius", &constants.R, 0, 5.0);
        }
        else if(surface == SURFACE_CUBE){
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
        }else if(surface == SURFACE_CYLINDER){
            auto& constants = pCylinder.constants;
            tessellationLevelUI(constants.tessLevelOuter, constants.tessLevelInner, constants.u, constants.v);
            ImGui::SliderFloat("radius", &constants.radius, 0.1, 5.0);
            ImGui::SliderFloat("height", &constants.height, 0.5, 5.0);
        }else if(surface == SURFACE_CONE){
            auto& constants = pCone.constants;
            tessellationLevelUI(constants.tessLevelOuter, constants.tessLevelInner, constants.u, constants.v);
            ImGui::SliderFloat("radius", &constants.radius, 0.1, 5.0);
            ImGui::SliderFloat("height", &constants.height, 0.5, 5.0);
        }else if(surface == SURFACE_SWEPT){
            auto& constants = pSurfaceRevolution.constants;
            tessellationLevelUI(constants.tessLevelOuter, constants.tessLevelInner, constants.u, constants.v);
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

    if((curve & CURVES_WITH_CONTROL_POINTS) && contextPopup.active){
        ImGui::Begin(curve == CURVE_HERMITE ? "Hermite Curve" : "Bezier Curve");
        ImGui::SetWindowSize({0, 0});
        ImGui::SetWindowPos(contextPopup.pos);
        if(curve == CURVE_HERMITE && ImGui::Selectable("add points")){
            contextPopup.active = false;
            addingPoints = true;
        }
        if(ImGui::Selectable(fmt::format("{}{}", !showPoints ? "" : "don't ", "show points").c_str())){
            showPoints = !showPoints;
            contextPopup.active = false;
        }
        if(ImGui::Selectable(fmt::format("{}{}", !showTangentLines ? "" : "don't ", "show tangent lines").c_str())){
            showTangentLines = !showTangentLines;
            contextPopup.active = false;
        }
        if(curve == CURVE_HERMITE && dirty && ImGui::Selectable("save points")){
            contextPopup.active = false;
            savePoints(hermiteCurve);
        }
        ImGui::End();

    }


    plugin(IM_GUI_PLUGIN).draw(commandBuffer);
}

void CurvesSurfaceDemo::tessellationLevelUI(float &outer, float &inner, float& u, float& v) {
    static bool link = true;
    auto outerUpdated = ImGui::SliderFloat("Outer tess levels", &outer, 1, 128); ImGui::SameLine();
    ImGui::Checkbox("link", &link);
    auto innerUpdated = ImGui::SliderFloat("Inner tess levels", &inner, 1, 128);
    ImGui::SliderFloat("u", &u, 0, 1);
    ImGui::SliderFloat("v", &v, 0, 1);

    if(link){
        if(outerUpdated) inner = outer;
        if(innerUpdated) outer = inner;
    }
}


void CurvesSurfaceDemo::renderBezierSurface(VkCommandBuffer commandBuffer){
    switch(surface){
        case SURFACE_TEAPOT :
            renderBezierSurface(commandBuffer, teapot);
            break;
        case SURFACE_TEACUP:
            renderBezierSurface(commandBuffer, teacup);
            break;
        case SURFACE_TEASPOON:
            renderBezierSurface(commandBuffer, teaspoon);
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

    if(pSphereSurface.xray.enabled){
        static std::array<int, 2> levels{10, 360};
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pSphereSurface.xray.pipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pSphereSurface.xray.layout, 0,1,    &mvp.descriptorSet, 0, VK_NULL_HANDLE);
        vkCmdPushConstants(commandBuffer, pSphereSurface.xray.layout, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, 0,sizeof(levels), levels.data());
        vkCmdPushConstants(commandBuffer, pSphereSurface.xray.layout, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, sizeof(levels),sizeof(pSphereSurface.constants), &pSphereSurface.constants);
    }else {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pSphereSurface.pipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pSphereSurface.layout, 0,COUNT(descriptorSets), descriptorSets.data(), 0, VK_NULL_HANDLE);
        vkCmdPushConstants(commandBuffer, pSphereSurface.layout, TESSELLATION_SHADER_STAGES_ALL, 0,sizeof(pSphereSurface.constants), &pSphereSurface.constants);
    }
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, sphere.points, &offset);
    vkCmdBindIndexBuffer(commandBuffer, sphere.indices, 0, VK_INDEX_TYPE_UINT16);
    vkCmdDrawIndexed(commandBuffer, sphere.indices.size/sizeof(int16_t), 1, 0, 0, 0);
}

void CurvesSurfaceDemo::renderTorusSurface(VkCommandBuffer commandBuffer) {
    VkDeviceSize offset = 0;
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pTorusSurface.pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pTorusSurface.layout, 0,COUNT(descriptorSets), descriptorSets.data(), 0, VK_NULL_HANDLE);
    vkCmdPushConstants(commandBuffer, pTorusSurface.layout, TESSELLATION_SHADER_STAGES_ALL, 0,sizeof(pTorusSurface.constants), &pTorusSurface.constants);
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, sphere.points, &offset);
    vkCmdBindIndexBuffer(commandBuffer, sphere.indices, 0, VK_INDEX_TYPE_UINT16);
    vkCmdDrawIndexed(commandBuffer, sphere.indices.size/sizeof(int16_t), 1, 0, 0, 0);
}

void CurvesSurfaceDemo::renderCylinderSurface(VkCommandBuffer commandBuffer) {
    VkDeviceSize offset = 0;
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pCylinder.pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pCylinder.layout, 0,COUNT(descriptorSets), descriptorSets.data(), 0, VK_NULL_HANDLE);
    vkCmdPushConstants(commandBuffer, pCylinder.layout, TESSELLATION_SHADER_STAGES_ALL, 0,sizeof(pCylinder.constants), &pCylinder.constants);
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, sphere.points, &offset);
    vkCmdBindIndexBuffer(commandBuffer, sphere.indices, 0, VK_INDEX_TYPE_UINT16);
    vkCmdDrawIndexed(commandBuffer, sphere.indices.size/sizeof(int16_t), 1, 0, 0, 0);
}

void CurvesSurfaceDemo::renderCone(VkCommandBuffer commandBuffer) {
    VkDeviceSize offset = 0;
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pCone.pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pCylinder.layout, 0,COUNT(descriptorSets), descriptorSets.data(), 0, VK_NULL_HANDLE);
    vkCmdPushConstants(commandBuffer, pCone.layout, TESSELLATION_SHADER_STAGES_ALL, 0,sizeof(pCone.constants), &pCone.constants);
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

void CurvesSurfaceDemo::renderControlPoints(VkCommandBuffer commandBuffer,  Patch& patch)  {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pPoints.pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pPoints.layout, 0, 1, &mvp.descriptorSet, 0, VK_NULL_HANDLE);
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, patch.points, &offset);
    vkCmdDraw(commandBuffer, patch.numPoints, 1, 0, 0);
}

void CurvesSurfaceDemo::renderTangentLines(VkCommandBuffer commandBuffer, Patch &patch, VulkanBuffer *indices, uint32_t indexCount) {
    indices = indices ? indices : &patch.indices;
    indexCount = indexCount != 0 ? indexCount : indices->size/sizeof(int16_t);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pTangentLines.pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pTangentLines.layout, 0, 1, &mvp.descriptorSet, 0, VK_NULL_HANDLE);
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, patch.points, &offset);
    vkCmdDraw(commandBuffer, indices->size/sizeof(int16_t), 1, 0, 0);
}


void CurvesSurfaceDemo::update(float time) {
    if(!ImGui::IsAnyItemActive()){
        cameraController->update(time);
        auto cam = mode == MODE_CURVES ? camera2d : cameraController->cam();
        mvp.buffer.copy(&cam, sizeof(Camera));
        globalUBo.buffer.copy(&globalConstants, sizeof(globalConstants));
    }
}

void CurvesSurfaceDemo::checkAppInputs() {
    cameraController->processInput();

    if(mode == MODE_CURVES && (curve & CURVES_WITH_CONTROL_POINTS) != 0) {
        if(addingPoints){
            if(addPointAction->isPressed() && addPointAction->state == Action::State::RELEASED){
                auto point = selectPoint();
                int index;
                if(!isPointOnScreen(point, index)){
                    pointsAdded.push_back(point);
                    auto pointsToAdd = hermiteCurve.numPoints == 0 ? 4 : 2;
                    if(pointsAdded.size() == pointsToAdd){
                        addingPoints = false;
                        updateHermiteCurve();
                    }
                }
            }
        }else{
            if (movePointAction->isPressed() && selectedPoint == -1) {
                auto point = selectPoint();
                 isPointOnScreen(point, selectedPoint);
            }
            if (movePointAction->isPressed() && selectedPoint != -1) {
                auto position = selectPoint();
                movePoint(selectedPoint, position);
            } else {
                selectedPoint = -1;
            }
        }
    }

    if(mode == MODE_CURVES && (curve & CURVES_WITH_CONTROL_POINTS) && ImGui::IsMouseReleased(ImGuiMouseButton_Right)){
        if(contextPopup.active){
            contextPopup.active = false;
        }else{
            contextPopup.active = true;
            contextPopup.pos = {mouse.position.x, mouse.position.y};
        }
    }

}

void CurvesSurfaceDemo::cleanup() {
    if(point.handle){
        point.source->points.unmap();
    }
}

void CurvesSurfaceDemo::onPause() {
    VulkanBaseApp::onPause();
}


void CurvesSurfaceDemo::movePoint(int pointIndex, glm::vec3 position) {
    assert(point.handle);
    point.handle[pointIndex] = position;
    dirty = true;
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
    cameraController = std::make_unique<OrbitingCameraController>(static_cast<InputManager&>(*this), settings);
    updateCameras();
}

void CurvesSurfaceDemo::updateCameras() {
    auto as = swapChain.aspectRatio();
    cameraController->perspective(as);
    auto s = 1/as;
    camera2d.view = glm::scale(glm::mat4(1), {s, 1, 1});
    camera2d.proj = vkn::ortho(0, 1, 0 , 1, -1, 1);
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
    bezierCurve.numPoints = 4;
    indices = std::vector<uint16_t>{1, 3};
}

bool CurvesSurfaceDemo::isPointOnScreen(glm::vec3 aPoint, int& position) {
    auto points = point.handle;
    float minDist = std::numeric_limits<float>::max();
    auto numPoints = point.numPoints;
    for(int i = 0; i < numPoints; i++){
        auto point = points[i];
        auto dist = glm::distance(aPoint, point);
        if(dist < minDist){
            position = i;
            minDist = dist;
        }
    }
    if(position != -1 && minDist < 0.01){
        return true;
    }
    position = -1;
    return false;
}

void CurvesSurfaceDemo::createPatches() {
    createSpherePatch();
    createCubePatch();
    createBezierCurvePatch();
    createArcPatch();
    loadBezierPatches();
    createIcoSpherePatch();
    createHermitePatch();
}

void CurvesSurfaceDemo::loadBezierPatches() {
//    loadPatch("teapot", teapot);
//    auto [nodes, indices] = teapotPatch();
//    teapot.points = device.createDeviceLocalBuffer(nodes.data(), BYTE_SIZE(nodes), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
//    teapot.indices = device.createDeviceLocalBuffer(indices.data(), BYTE_SIZE(indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    loadPatch("teapot", teapot);
    loadPatch("teacup", teacup);
    loadPatch("teaspoon", teaspoon);
    loadProfile();
}

void CurvesSurfaceDemo::loadPatch(const std::string &name, Patch &patch) {
    spdlog::info("loading patch: {}", name);
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

    patch.points = device.createCpuVisibleBuffer(nodes.data(), BYTE_SIZE(nodes), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

    std::vector<uint16_t> rectangles(nodes.size());
    std::iota(begin(rectangles), end(rectangles), 0);
    patch.indices = device.createDeviceLocalBuffer(rectangles.data(), BYTE_SIZE(rectangles), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
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

void CurvesSurfaceDemo::loadTexture() {
    textures::fromFile(device, vulkanImage.texture, resource("albedo.png"), true);
}

void CurvesSurfaceDemo::createArcPatch() {
    std::vector<glm::vec3> points(2, glm::vec3(0));
    std::vector<int16_t> indices(2);
    std::iota(begin(indices),  end(indices), 0);
    arc.points = device.createCpuVisibleBuffer(points.data(), BYTE_SIZE(points), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    arc.indices = device.createCpuVisibleBuffer(indices.data(), BYTE_SIZE(indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
}

void CurvesSurfaceDemo::renderCurve(VkCommandBuffer commandBuffer) {
    switch (curve) {
        case CURVE_BEZIER:
            pBezierCurve.constants.levels[1] = curveResolution;
            updatePointHandle(bezierCurve);
            renderCurve(commandBuffer, pBezierCurve.pipeline, pBezierCurve.layout, bezierCurve, &pBezierCurve.constants, sizeof(pBezierCurve.constants), VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);
            if(showPoints){
                renderControlPoints(commandBuffer, bezierCurve);
            }
            if(showTangentLines) {
                renderTangentLines(commandBuffer, bezierCurve);
            }
            break;
        case CURVE_ARC:
            pArcCurve.constants.levels[1] = curveResolution;
            renderCurve(commandBuffer, pArcCurve.pipeline, pArcCurve.layout, arc, &pArcCurve.constants, sizeof(pArcCurve.constants), TESSELLATION_SHADER_STAGES_ALL);
            break;
        case CURVE_HERMITE:
            pHermiteCurve.constants.levels[1] = curveResolution;
            updatePointHandle(hermiteCurve);
            renderCurve(commandBuffer, pHermiteCurve.pipeline, pHermiteCurve.layout, hermiteCurve, &pHermiteCurve.constants, sizeof(pHermiteCurve.constants), VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, hermiteCurve.numIndices);
            if(showPoints){
                renderControlPoints(commandBuffer, hermiteCurve);
            }
            if(showTangentLines) {
                renderTangentLines(commandBuffer, hermiteCurve);
            }
            break;
        default:
            spdlog::warn("curve type {} not supported", curve);
    }
}

void CurvesSurfaceDemo::updatePointHandle(Patch &patch) {
    if(point.handle){
        point.source->points.unmap();
    }
    point.handle = reinterpret_cast<glm::vec3*>(patch.points.map());
    point.source = &patch;
    point.numPoints = patch.numPoints;
}

void
CurvesSurfaceDemo::renderCurve(VkCommandBuffer commandBuffer, VulkanPipeline &pipeline, VulkanPipelineLayout &layout,
                               Patch &patch, void *constants, uint32_t constantsSize, VkShaderStageFlags shaderStageFlags, int numIndices) {

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &mvp.descriptorSet, 0, VK_NULL_HANDLE);
    if(constants){
        vkCmdPushConstants(commandBuffer, layout, shaderStageFlags, 0, constantsSize, constants);
    }
    numIndices = numIndices == 0 ? patch.indices.size/sizeof(uint16_t) : numIndices;
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, patch.points, &offset);
    vkCmdBindIndexBuffer(commandBuffer, patch.indices, 0, VK_INDEX_TYPE_UINT16);
    vkCmdDrawIndexed(commandBuffer, numIndices, 1, 0, 0, 0); // FIXME pass the indexCount in

}

void CurvesSurfaceDemo::createHermitePatch() {
    std::vector<glm::vec3> points(64);
//    points[0] = {-0.814, 0.07, 0};
//    points[1] = {-0.333, 0.55, 0};
//    points[2] = {0.904, 0.914, 0};
//    points[3] = {0, 0.794, 0};

    std::vector<uint16_t> indices(64);
//    std::iota(begin(indices), end(indices), 0);

    auto vasePath = resource("mushroom.txt");
    std::ifstream fin(vasePath);
    if(!fin.good()) throw  std::runtime_error{fmt::format("unable to open {}}", vasePath)};

    int numPoints = -1;
    while(!fin.eof()){
        glm::vec3 point{0};
        fin >> point.x >> point.y >> point.z;
        points[++numPoints] = point;
    }

    indices[0] = 0;
    indices[1] = 1;
    indices[2] = 2;
    indices[3] = 3;
    int numIndices = 4;
    for(int i = 4; i < numPoints; i += 2){
        indices[numIndices] = indices[numIndices - 2];
        indices[numIndices + 1] = indices[numIndices - 1];
        indices[numIndices + 2] = indices[numIndices - 1] + 1;
        indices[numIndices + 3] = indices[numIndices - 1] + 2;
        numIndices += 4;
    }
    spdlog::info("{} vase points loaded", numIndices);
    fin.close();


    hermiteCurve.points = device.createCpuVisibleBuffer(points.data(), BYTE_SIZE(points), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    hermiteCurve.indices = device.createCpuVisibleBuffer(indices.data(), BYTE_SIZE(indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    hermiteCurve.numPoints = numPoints;
    hermiteCurve.numIndices = numIndices;

//    hermiteCurve.numPoints = 0;
//    hermiteCurve.numIndices = 0;

}

void CurvesSurfaceDemo::updateHermiteCurve() {
    auto back = hermiteCurve.numPoints;

    auto points = reinterpret_cast<glm::vec3*>(point.handle);
    points[back] = pointsAdded[0];
    points[back + 1] = pointsAdded[1];

    auto noPoints = point.source->numPoints == 0;
    if(noPoints){
        points[back + 2] = pointsAdded[2];
        points[back + 3] = pointsAdded[3];
    }

    auto prevPointIndex = noPoints ? 0 : 2;
    auto lastIndex = back - prevPointIndex;
    back = hermiteCurve.numIndices;
    auto indices = reinterpret_cast<uint16_t*>(hermiteCurve.indices.map());
    indices[back] = lastIndex;
    indices[back+1] = lastIndex + 1;
    indices[back+2] = lastIndex + 2;
    indices[back+3] = lastIndex + 3;


    hermiteCurve.numPoints += noPoints ? 4 : 2;
    hermiteCurve.numIndices += 4;
    pointsAdded.clear();

    for(int i = 0; i < hermiteCurve.numIndices; i++){
        spdlog::info("index[{}] => {}", i, indices[i]);
    }

    for(int i = 0; i < hermiteCurve.numPoints; i++){
        spdlog::info("point[{}] => {}", i, points[i]);
    }
    hermiteCurve.indices.unmap();
    dirty = true;
}

void CurvesSurfaceDemo::savePoints(Patch& patch) {
    std::ofstream fout{"hermite.txt"};
    if(fout.bad()){
        spdlog::warn("unable to save points");
        return;
    }
    auto points = reinterpret_cast<glm::vec3*>(patch.points.map());
    for(int i = 0; i < patch.numPoints; i++){
        auto p = points[i];
        fout << fmt::format("{} {} {}\n", p.x, p.y, p.z );
    }
    fout.close();
    dirty = false;
    spdlog::info("point saved");
}

void CurvesSurfaceDemo::renderSweptSurface(VkCommandBuffer commandBuffer) {
    static std::array<VkDescriptorSet, 4> sets;
    sets[0] = descriptorSets[0];
    sets[1] = descriptorSets[1];
    sets[2] = descriptorSets[2];
    sets[3] = profileCurve.descriptorSet;
    VkDeviceSize offset = 0;
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pSurfaceRevolution.pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pSurfaceRevolution.layout, 0, COUNT(sets), sets.data(), 0, VK_NULL_HANDLE);
    vkCmdPushConstants(commandBuffer, pSurfaceRevolution.layout, TESSELLATION_SHADER_STAGES_ALL, 0, sizeof(pSurfaceRevolution.constants), &pSurfaceRevolution.constants);
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, sweptSurface.points, &offset);
    vkCmdBindIndexBuffer(commandBuffer, sweptSurface.indices, 0, VK_INDEX_TYPE_UINT16);
    vkCmdDrawIndexed(commandBuffer, sweptSurface.indices.size/sizeof(int16_t), 1, 0, 0, 0);
}

void CurvesSurfaceDemo::loadProfile() {
    auto vasePath = resource("mushroom.txt");
    std::ifstream fin(vasePath);
    if(!fin.good()) throw  std::runtime_error{fmt::format("unable to open {}}", vasePath)};

    std::vector<glm::vec4> points(64);
    int numPoints = -1;
    while(!fin.eof()){
        glm::vec4 point{1};
        fin >> point.x >> point.y >> point.z;
        points[++numPoints] = point;
    }

    std::vector<int> indices(64);
    indices[0] = 0;
    indices[1] = 1;
    indices[2] = 2;
    indices[3] = 3;
    int numIndices = 4;
    for(int i = 4; i < numPoints; i += 2){
        indices[numIndices] = indices[numIndices - 2];
        indices[numIndices + 1] = indices[numIndices - 1];
        indices[numIndices + 2] = indices[numIndices - 1] + 1;
        indices[numIndices + 3] = indices[numIndices - 1] + 2;
        numIndices += 4;
    }
    spdlog::info("{} vase points loaded", numIndices);
    fin.close();

    profileCurve.vertices = device.createDeviceLocalBuffer(points.data(), BYTE_SIZE(points), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    profileCurve.indices = device.createDeviceLocalBuffer(indices.data(), BYTE_SIZE(indices), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    pSurfaceRevolution.constants.numPoints = numIndices;

    auto numLines = numIndices/4;
    sweptSurface.points = device.createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, sizeof(glm::vec3) * 4 * numLines);
    std::vector<uint16_t> ssIndices(numLines * 4);
    std::iota(begin(ssIndices), end(ssIndices), 0);
    sweptSurface.indices = device.createCpuVisibleBuffer(ssIndices.data(), BYTE_SIZE(ssIndices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
}

int main(){
    try{

        Settings settings;
        settings.depthTest = true;
        settings.msaaSamples = VK_SAMPLE_COUNT_16_BIT;
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