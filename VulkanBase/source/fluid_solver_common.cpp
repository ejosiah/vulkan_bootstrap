#include "fluid_solver_common.h"
#include "GraphicsPipelineBuilder.hpp"
#include "Vertex.h"

FluidSolver::FluidSolver(VulkanDevice *device, VulkanDescriptorPool *descriptorPool,
                         VulkanRenderPass *displayRenderPass, FileManager *fileManager, glm::uvec3 gridSize)
: device(device)
, descriptorPool(descriptorPool)
, displayRenderPass(displayRenderPass)
, fileManager(fileManager)
, width(gridSize.x)
, height(gridSize.y)
, depth(gridSize.z)
{

}

void FluidSolver::initBuffers() {
    auto [size, globalConstants] = getGlobalConstants();
    globalConstantsBuffer = device->createCpuVisibleBuffer(globalConstants, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    debugBuffer = device->createBuffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_TO_CPU, sizeof(glm::vec4) * width * height, "debug");
}

void FluidSolver::createRenderPass(){
    VkAttachmentDescription attachmentDescription{
            0,
            VK_FORMAT_R32G32B32A32_SFLOAT,
            VK_SAMPLE_COUNT_1_BIT,
            VK_ATTACHMENT_LOAD_OP_CLEAR,
            VK_ATTACHMENT_STORE_OP_STORE,
            VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            VK_ATTACHMENT_STORE_OP_STORE,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    std::vector<SubpassDescription> subpasses(1);
    subpasses[0].colorAttachments.push_back({0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});

    std::vector<VkSubpassDependency> dependencies(2);

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    renderPass = device->createRenderPass({attachmentDescription}, subpasses, dependencies);
    device->setName<VK_OBJECT_TYPE_RENDER_PASS>("fluid_simulation", renderPass.renderPass);
}

void FluidSolver::createSamplers() {
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST ;

    valueSampler = device->createSampler(samplerInfo);

    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    linearSampler = device->createSampler(samplerInfo);
}

void FluidSolver::createPipelines() {
//    @formatter:off
    auto builder = device->graphicsPipelineBuilder();
    arrows.pipeline =
        builder
            .allowDerivatives()
            .shaderStage()
                .vertexShader(resource("vectorField.vert.spv"))
                .fragmentShader(resource("vectorField.frag.spv"))
            .vertexInputState()
                .addVertexBindingDescription(0, sizeof(Vector), VK_VERTEX_INPUT_RATE_VERTEX)
                .addVertexAttributeDescription(0, 0, VK_FORMAT_R32G32_SFLOAT, offsetOf(Vector, vertex))
                .addVertexAttributeDescription(1, 0, VK_FORMAT_R32G32_SFLOAT, offsetOf(Vector, position))
            .inputAssemblyState()
                .triangles()
            .viewportState()
                .viewport()
                    .origin(0, 0)
                    .dimension(width, height)
                    .minDepth(0)
                    .maxDepth(1)
                .scissor()
                    .offset(0, 0)
                    .extent(width, height)
                .add()
            .rasterizationState()
                .cullBackFace()
                .frontFaceCounterClockwise()
                .polygonModeFill()
                .multisampleState()
            .rasterizationSamples(VK_SAMPLE_COUNT_1_BIT)
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
                .addDescriptorSetLayout(textureSetLayout)
            .renderPass(*displayRenderPass)
            .subpass(0)
            .name("vector_field")
        .build(arrows.layout);

    screenQuad.pipeline =
        builder
            .basePipeline(arrows.pipeline)
            .shaderStage()
                .vertexShader(resource("quad.vert.spv"))
                .fragmentShader(resource("quad.frag.spv"))
            .vertexInputState().clear()
                .addVertexBindingDescriptions(ClipSpace::bindingDescription())
                .addVertexAttributeDescriptions(ClipSpace::attributeDescriptions())
            .inputAssemblyState()
                .triangleStrip()
            .layout().clear()
                .addDescriptorSetLayout(textureSetLayout)
            .renderPass(renderPass)
            .name("fullscreen_quad")
        .build(screenQuad.layout);

    advectPipeline.pipeline =
        builder
            .basePipeline(arrows.pipeline)
            .shaderStage()
                .vertexShader(resource("quad.vert.spv"))
                .fragmentShader(resource("advect.frag.spv"))
            .layout().clear()
                .addDescriptorSetLayouts({globalConstantsSet, textureSetLayout, advectTextureSet, samplerSet})
            .renderPass(renderPass)
            .name("advect")
        .build(advectPipeline.layout);

    divergence.pipeline =
        builder
            .shaderStage()
                .fragmentShader(resource("divergence.frag.spv"))
            .layout().clear()
                .addDescriptorSetLayouts( { globalConstantsSet, textureSetLayout})
            .renderPass(renderPass)
            .name("divergence")
        .build(divergence.layout);

    divergenceFree.pipeline =
        builder
            .shaderStage()
                .fragmentShader(resource("divergence_free_field.frag.spv"))
            .layout().clear()
                .addDescriptorSetLayouts({globalConstantsSet, textureSetLayout, textureSetLayout})
            .renderPass(renderPass)
            .name("divergence_free_field")
        .build(divergenceFree.layout);

    jacobi.pipeline =
        builder
            .shaderStage()
                .fragmentShader(resource("jacobi.frag.spv"))
            .layout().clear()
                .addDescriptorSetLayouts({globalConstantsSet, textureSetLayout, textureSetLayout})
                .addPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(jacobi.constants))
            .renderPass(renderPass)
            .name("jacobi")
        .build(jacobi.layout);

    addSourcePipeline.pipeline =
        builder
            .shaderStage()
                .fragmentShader(resource("add_sources.frag.spv"))
            .layout().clear()
                .addDescriptorSetLayouts({textureSetLayout, textureSetLayout})
                .addPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(addSourcePipeline.constants))
            .renderPass(renderPass)
            .name("add_sources")
        .build(addSourcePipeline.layout);

    vorticity.pipeline =
        builder
            .shaderStage()
                .fragmentShader(resource("vorticity.frag.spv"))
            .layout().clear()
                .addDescriptorSetLayouts({globalConstantsSet, textureSetLayout})
            .renderPass(renderPass)
            .name("vorticity")
        .build(vorticity.layout);

    vorticityForce.pipeline =
        builder
            .shaderStage()
                .fragmentShader(resource("vorticity_force.frag.spv"))
            .layout().clear()
                .addDescriptorSetLayouts({globalConstantsSet, textureSetLayout, textureSetLayout})
                .addPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(vorticityForce.constants))
            .renderPass(renderPass)
            .name("vorticity_force")
        .build(vorticityForce.layout);

//    @formatter:on
}

void FluidSolver::createDescriptorSetLayouts() {
    globalConstantsSet =
            device->descriptorSetLayoutBuilder()
                    .name("global_constants")
                    .binding(0)
                    .descriptorType(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
                    .descriptorCount(1)
                    .shaderStages(VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT)
                    .createLayout();

    textureSetLayout =
            device->descriptorSetLayoutBuilder()
                    .name("texture_set")
                    .binding(0)
                    .descriptorType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                    .descriptorCount(1)
                    .shaderStages(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT)
                    .immutableSamplers(valueSampler)
                    .createLayout();

    samplerSet =
            device->descriptorSetLayoutBuilder()
                    .name("advect_sampler")
                    .binding(0)
                    .descriptorType(VK_DESCRIPTOR_TYPE_SAMPLER)
                    .descriptorCount(1)
                    .shaderStages(VK_SHADER_STAGE_FRAGMENT_BIT)
                    .createLayout();

    advectTextureSet =
            device->descriptorSetLayoutBuilder()
                    .name("advect_texture")
                    .binding(0)
                    .descriptorType(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
                    .descriptorCount(1)
                    .shaderStages(VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT)
                    .createLayout();

    auto sets = descriptorPool->allocate(
            {
                    textureSetLayout, textureSetLayout, advectTextureSet, advectTextureSet
                    , textureSetLayout, textureSetLayout, advectTextureSet, advectTextureSet
                    , textureSetLayout, textureSetLayout, advectTextureSet, advectTextureSet
                    , textureSetLayout, textureSetLayout, textureSetLayout, samplerSet
                    , globalConstantsSet
            });

    vectorField.descriptorSet[0] = sets[0];
    vectorField.descriptorSet[1] = sets[1];
    vectorField.advectDescriptorSet[0] = sets[2];
    vectorField.advectDescriptorSet[1] = sets[3];

    pressureField.descriptorSet[0] = sets[4];
    pressureField.descriptorSet[1] = sets[5];
    pressureField.advectDescriptorSet[0] = sets[6];
    pressureField.advectDescriptorSet[1] = sets[7];

    forceField.descriptorSet[0] = sets[8];
    forceField.descriptorSet[1] = sets[9];
    forceField.advectDescriptorSet[0] = sets[10];
    forceField.advectDescriptorSet[1] = sets[11];

    diffuseHelper.solutionDescriptorSet = sets[12];
    divergenceField.descriptorSet[0] = sets[13];
    vorticityField.descriptorSet[0] = sets[14];
    samplerDescriptorSet = sets[15];
    globalConstantsDescriptorSet = sets[16];

    device->setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>(fmt::format("{}_{}", "vector_field", 0), vectorField.descriptorSet[0]);
    device->setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>(fmt::format("{}_{}", "vector_field", 1), vectorField.descriptorSet[1]);
    device->setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>(fmt::format("{}_{}", "advect_vector_field", 0), vectorField.advectDescriptorSet[0]);
    device->setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>(fmt::format("{}_{}", "advect_vector_field", 1), vectorField.advectDescriptorSet[1]);


    device->setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>(fmt::format("{}_{}", "pressure_field", 0), pressureField.descriptorSet[0]);
    device->setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>(fmt::format("{}_{}", "pressure_field", 1), pressureField.descriptorSet[1]);
    device->setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>(fmt::format("{}_{}", "advect_pressure_field", 0), pressureField.advectDescriptorSet[0]);
    device->setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>(fmt::format("{}_{}", "advect_pressure_field", 1), pressureField.advectDescriptorSet[1]);


    device->setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>(fmt::format("{}_{}", "force_field", 0), forceField.descriptorSet[0]);
    device->setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>(fmt::format("{}_{}", "force_field", 1), forceField.descriptorSet[1]);
    device->setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>(fmt::format("{}_{}", "advect_force_field", 0), forceField.advectDescriptorSet[0]);
    device->setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>(fmt::format("{}_{}", "advect_force_field", 1), forceField.advectDescriptorSet[1]);

    device->setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>("divergence_field", divergenceField.descriptorSet[0]);
    device->setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>("diffuse_solution_container", diffuseHelper.solutionDescriptorSet);
    device->setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>("vorticity_field", vorticityField.descriptorSet[0]);
    device->setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>("global_constants", globalConstantsDescriptorSet);
}

void FluidSolver::initSimData(){
    std::vector<glm::vec4> allocation( width * height * depth );

    textures::create(*device, vectorField.texture[0], VK_IMAGE_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT
            , allocation.data(), {width, height, depth}, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
            , sizeof(float));
    textures::create(*device, vectorField.texture[1], VK_IMAGE_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT
            , allocation.data(), {width, height, depth}, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
            , sizeof(float));

    textures::create(*device, divergenceField.texture[0], VK_IMAGE_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT, allocation.data(), {width, height, depth}, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
            , sizeof(float));
    textures::create(*device, pressureField.texture[0], VK_IMAGE_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT, allocation.data(), {width, height, depth}, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
            , sizeof(float));
    textures::create(*device, pressureField.texture[1], VK_IMAGE_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT, allocation.data(), {width, height, depth}, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
            , sizeof(float));

    textures::create(*device, forceField.texture[0], VK_IMAGE_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT, allocation.data(), {width, height, depth}, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
            , sizeof(float));
    textures::create(*device, forceField.texture[1], VK_IMAGE_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT, allocation.data(), {width, height, depth}, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
            , sizeof(float));

    textures::create(*device, vorticityField.texture[0], VK_IMAGE_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT, allocation.data(), {width, height, depth}, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
            , sizeof(float));

    textures::create(*device, diffuseHelper.texture, VK_IMAGE_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT, allocation.data(), {width, height, depth}, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
            , sizeof(float));

    divergenceField.framebuffer[0] = device->createFramebuffer(renderPass, { divergenceField.texture[0].imageView }, width, height, depth);
    device->setName<VK_OBJECT_TYPE_FRAMEBUFFER>("divergence_field", divergenceField.framebuffer[0].frameBuffer);

    forceField.framebuffer[0] = device->createFramebuffer(renderPass, { forceField.texture[0].imageView }, width, height, depth);
    forceField.framebuffer[1] = device->createFramebuffer(renderPass, { forceField.texture[1].imageView }, width, height, depth);
    device->setName<VK_OBJECT_TYPE_FRAMEBUFFER>(fmt::format("{}_{}", "force_field", 0), forceField.framebuffer[0].frameBuffer);
    device->setName<VK_OBJECT_TYPE_FRAMEBUFFER>(fmt::format("{}_{}", "force_field", 1), forceField.framebuffer[1].frameBuffer);

    vorticityField.framebuffer[0] = device->createFramebuffer(renderPass, { vorticityField.texture[0].imageView }, width, height, depth);
    device->setName<VK_OBJECT_TYPE_FRAMEBUFFER>("vorticity_field", vorticityField.framebuffer[0].frameBuffer);

    for(auto i = 0; i < 2; i++){
        vectorField.framebuffer[i] = device->createFramebuffer(renderPass, { vectorField.texture[i].imageView }, width, height, depth);
        device->setName<VK_OBJECT_TYPE_FRAMEBUFFER>(fmt::format("{}_{}", "vector_field", i), vectorField.framebuffer[i].frameBuffer);

        pressureField.framebuffer[i] = device->createFramebuffer(renderPass, { pressureField.texture[i].imageView }, width, height, depth);
        device->setName<VK_OBJECT_TYPE_FRAMEBUFFER>(fmt::format("{}_{}", "pressure_field", i), pressureField.framebuffer[i].frameBuffer);
    }
}

void FluidSolver::initViewVectors() {
    constexpr int interval = 30;
    std::vector<glm::vec2> triangles{
            {0, 0.2}, {1, 0}, {0, -0.2}
    };
    std::vector<Vector> vectors;
    for(auto i = interval/2; i < height; i += interval){
        for(auto j = interval/2; j < width; j += interval){
            for(int k = 0; k < 3; k++){
                auto vertex = triangles[k];
                glm::vec2 position{2 * (float(j)/float(width)) - 1
                        , 2 * (float(i)/float(height)) - 1};
                vectors.push_back(Vector{vertex, position});
            }
        }
    }

    arrows.vertexBuffer = device->createDeviceLocalBuffer(vectors.data(), BYTE_SIZE(vectors), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    arrows.numArrows = vectors.size();
}

void FluidSolver::initFullScreenQuad() {
    auto quad = ClipSpace::Quad::positions;
    screenQuad.vertices = device->createDeviceLocalBuffer(quad.data(), BYTE_SIZE(quad), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

std::string FluidSolver::resource(const std::string &name) {
    auto res = fileManager->getFullPath(name);
    assert(res.has_value());
    return res->string();
}

void FluidSolver::withRenderPass(VkCommandBuffer commandBuffer, const VulkanFramebuffer &framebuffer
        , GpuProcess &&process, glm::vec4 clearColor) const {

    static std::array<VkClearValue, 1> clearValues;
    clearValues[0].color = {clearColor.b, clearColor.g, clearColor.b, clearColor.a};

    VkRenderPassBeginInfo rPassInfo = initializers::renderPassBeginInfo();
    rPassInfo.clearValueCount = COUNT(clearValues);
    rPassInfo.pClearValues = clearValues.data();
    rPassInfo.framebuffer = framebuffer;
    rPassInfo.renderArea.offset = {0u, 0u};
    rPassInfo.renderArea.extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
    rPassInfo.renderPass = renderPass;

    vkCmdBeginRenderPass(commandBuffer, &rPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    process(commandBuffer);

    vkCmdEndRenderPass(commandBuffer);
}

void FluidSolver::createDescriptorSets(Quantity &quantity) {
    auto sets = descriptorPool->allocate(
            {
                    textureSetLayout, textureSetLayout, advectTextureSet, advectTextureSet
                    , textureSetLayout, textureSetLayout
            });

    quantity.field.descriptorSet[0] = sets[0];
    quantity.field.descriptorSet[1] = sets[1];
    quantity.field.advectDescriptorSet[0] = sets[2];
    quantity.field.advectDescriptorSet[1] = sets[3];

    quantity.source.descriptorSet[0] = sets[4];
    quantity.source.descriptorSet[1] = sets[5];

    updateDescriptorSet(quantity.field);
    updateDescriptorSet(quantity.source);
}

void FluidSolver::updateDescriptorSet(Field &field) {
    std::vector<VkWriteDescriptorSet> writes;

    VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr};
    write.dstSet = field.descriptorSet[0];
    write.dstBinding = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.descriptorCount = 1;
    VkDescriptorImageInfo info0{VK_NULL_HANDLE, field.texture[0].imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    write.pImageInfo = &info0;

    writes.push_back(write);

    VkDescriptorImageInfo info1{VK_NULL_HANDLE, field.texture[1].imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    if(field.descriptorSet[1]){
        write.dstSet = field.descriptorSet[1];
        write.pImageInfo = &info1;
        writes.push_back(write);

        if(field.advectDescriptorSet[0]) {
            write.dstSet = field.advectDescriptorSet[0];
            write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            write.pImageInfo = &info0;
            writes.push_back(write);

            write.dstSet = field.advectDescriptorSet[1];
            write.pImageInfo = &info1;
            writes.push_back(write);
        }
    }
    device->updateDescriptorSets(writes);
}

void FluidSolver::updateDescriptorSets() {
    updateUboDescriptorSets();
    updateDescriptorSet(vectorField);
    updateDescriptorSet(divergenceField);
    updateDescriptorSet(pressureField);
    updateDescriptorSet(forceField);
    updateDescriptorSet(vorticityField);
    updateDiffuseDescriptorSet();
    updateAdvectDescriptorSet();
}

void FluidSolver::updateUboDescriptorSets() {
    auto writes = initializers::writeDescriptorSets<1>();

    writes[0].dstSet = globalConstantsDescriptorSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[0].descriptorCount = 1;
    VkDescriptorBufferInfo gcInfo{globalConstantsBuffer, 0, VK_WHOLE_SIZE};
    writes[0].pBufferInfo = &gcInfo;

    device->updateDescriptorSets(writes);
}

void FluidSolver::updateDiffuseDescriptorSet() {
    auto writes = initializers::writeDescriptorSets<1>();
    writes[0].dstSet = diffuseHelper.solutionDescriptorSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[0].descriptorCount = 1;
    VkDescriptorImageInfo diffuseInfo{VK_NULL_HANDLE, diffuseHelper.texture.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    writes[0].pImageInfo = &diffuseInfo;
    device->updateDescriptorSets(writes);
}

void FluidSolver::updateAdvectDescriptorSet() {
    auto writes = initializers::writeDescriptorSets<1>();

    writes[0].dstSet = samplerDescriptorSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    writes[0].descriptorCount = 1;
    VkDescriptorImageInfo samplerInfo{linearSampler, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED};
    writes[0].pImageInfo = &samplerInfo;

    device->updateDescriptorSets(writes);

}