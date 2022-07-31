#include "fluid_solver_2d.h"
#include "Vertex.h"
#include "GraphicsPipelineBuilder.hpp"
#include "DescriptorSetBuilder.hpp"
#include <spdlog/spdlog.h>

FluidSolver2D::FluidSolver2D(VulkanDevice* device, VulkanDescriptorPool* descriptorPool, VulkanRenderPass* displayRenderPass, FileManager *fileManager, glm::vec2 gridSize)
: device(device)
, descriptorPool(descriptorPool)
, displayRenderPass(displayRenderPass)
, fileManager(fileManager)
, gridSize(gridSize)
, delta(1.0f/gridSize)
, width(static_cast<uint32_t>(gridSize.x))
, height(static_cast<uint32_t>(gridSize.y))
{
    constants.epsilon = 1.0f/gridSize.x;
}

void FluidSolver2D::init() {
    createSamplers();
    initViewVectors();
    createRenderPass();
    initSimData();
    initFullScreenQuad();
    createDescriptorSetLayouts();
    updateDescriptorSets();
    createPipelines();
    debugBuffer = device->createBuffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_TO_CPU, sizeof(glm::vec4) * width * height, "debug");
}

void FluidSolver2D::createSamplers() {
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

void FluidSolver2D::initViewVectors() {
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

void FluidSolver2D::createRenderPass(){
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

void FluidSolver2D::initSimData(){
    if(!vectorFieldSet){
        spdlog::warn("No vectorField set");
    }
    std::vector<glm::vec4> allocation( width * height );

    textures::create(*device, vectorField.texture[0], VK_IMAGE_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT
            , allocation.data(), {width, height, 1}, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
            , sizeof(float));
    textures::create(*device, vectorField.texture[1], VK_IMAGE_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT
            , allocation.data(), {width, height, 1}, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
            , sizeof(float));
    
    textures::create(*device, divergenceField.texture[0], VK_IMAGE_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT, allocation.data(), {width, height, 1}, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
            , sizeof(float));
    textures::create(*device, pressureField.texture[0], VK_IMAGE_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT, allocation.data(), {width, height, 1}, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
            , sizeof(float));
    textures::create(*device, pressureField.texture[1], VK_IMAGE_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT, allocation.data(), {width, height, 1}, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
            , sizeof(float));

    textures::create(*device, forceField.texture[0], VK_IMAGE_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT, allocation.data(), {width, height, 1}, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
            , sizeof(float));
    textures::create(*device, forceField.texture[1], VK_IMAGE_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT, allocation.data(), {width, height, 1}, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
            , sizeof(float));

    textures::create(*device, vorticityField.texture[0], VK_IMAGE_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT, allocation.data(), {width, height, 1}, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
            , sizeof(float));

    textures::create(*device, diffuseHelper.texture, VK_IMAGE_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT, allocation.data(), {width, height, 1}, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
            , sizeof(float));

    divergenceField.framebuffer[0] = device->createFramebuffer(renderPass, { divergenceField.texture[0].imageView }, width, height);
    device->setName<VK_OBJECT_TYPE_FRAMEBUFFER>("divergence_field", divergenceField.framebuffer[0].frameBuffer);

    forceField.framebuffer[0] = device->createFramebuffer(renderPass, { forceField.texture[0].imageView }, width, height);
    forceField.framebuffer[1] = device->createFramebuffer(renderPass, { forceField.texture[1].imageView }, width, height);
    device->setName<VK_OBJECT_TYPE_FRAMEBUFFER>(fmt::format("{}_{}", "force_field", 0), forceField.framebuffer[0].frameBuffer);
    device->setName<VK_OBJECT_TYPE_FRAMEBUFFER>(fmt::format("{}_{}", "force_field", 1), forceField.framebuffer[1].frameBuffer);

    vorticityField.framebuffer[0] = device->createFramebuffer(renderPass, { vorticityField.texture[0].imageView }, width, height);
    device->setName<VK_OBJECT_TYPE_FRAMEBUFFER>("vorticity_field", vorticityField.framebuffer[0].frameBuffer);

    for(auto i = 0; i < 2; i++){
        vectorField.framebuffer[i] = device->createFramebuffer(renderPass, { vectorField.texture[i].imageView }, width, height);
        device->setName<VK_OBJECT_TYPE_FRAMEBUFFER>(fmt::format("{}_{}", "vector_field", i), vectorField.framebuffer[i].frameBuffer);

        pressureField.framebuffer[i] = device->createFramebuffer(renderPass, { pressureField.texture[i].imageView }, width, height);
        device->setName<VK_OBJECT_TYPE_FRAMEBUFFER>(fmt::format("{}_{}", "pressure_field", i), pressureField.framebuffer[i].frameBuffer);
    }
}

void FluidSolver2D::initFullScreenQuad() {
    auto quad = ClipSpace::Quad::positions;
    spdlog::info("quad: {}", quad);
    screenQuad.vertices = device->createDeviceLocalBuffer(quad.data(), BYTE_SIZE(quad), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

void FluidSolver2D::createDescriptorSetLayouts() {
    textureSetLayout =
            device->descriptorSetLayoutBuilder()
                    .name("texture_set")
                    .binding(0)
                    .descriptorType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                    .descriptorCount(1)
                    .shaderStages(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
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
            .shaderStages(VK_SHADER_STAGE_FRAGMENT_BIT)
        .createLayout();

    auto sets = descriptorPool->allocate(
            {
                      textureSetLayout, textureSetLayout, advectTextureSet, advectTextureSet
                    , textureSetLayout, textureSetLayout, advectTextureSet, advectTextureSet
                    , textureSetLayout, textureSetLayout, advectTextureSet, advectTextureSet
                    , textureSetLayout, textureSetLayout, textureSetLayout, samplerSet,
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
}

void FluidSolver2D::createDescriptorSets(Quantity &quantity) {
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

void FluidSolver2D::updateDescriptorSets() {
    updateDescriptorSet(vectorField);
    updateDescriptorSet(divergenceField);
    updateDescriptorSet(pressureField);
    updateDescriptorSet(forceField);
    updateDescriptorSet(vorticityField);
    updateDiffuseDescriptorSet();
    updateAdvectDescriptorSet();
}

void FluidSolver2D::updateDescriptorSet(Field &field) {
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

void FluidSolver2D::updateDiffuseDescriptorSet() {
    auto writes = initializers::writeDescriptorSets<1>();
    writes[0].dstSet = diffuseHelper.solutionDescriptorSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[0].descriptorCount = 1;
    VkDescriptorImageInfo diffuseInfo{VK_NULL_HANDLE, diffuseHelper.texture.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    writes[0].pImageInfo = &diffuseInfo;
    device->updateDescriptorSets(writes);
}

void FluidSolver2D::updateAdvectDescriptorSet() {
    auto writes = initializers::writeDescriptorSets<1>();
    
    writes[0].dstSet = samplerDescriptorSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    writes[0].descriptorCount = 1;
    VkDescriptorImageInfo samplerInfo{linearSampler, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED};
    writes[0].pImageInfo = &samplerInfo;

    device->updateDescriptorSets(writes);

}

void FluidSolver2D::set(VulkanBuffer vectorFieldBuffer) {

    device->graphicsCommandPool().oneTimeCommand([&](auto commandBuffer){
        vectorField.texture[in].image.transitionLayout(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, DEFAULT_SUB_RANGE
                , VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT
                , VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {width, height, 1u};

        vkCmdCopyBufferToImage(commandBuffer, vectorFieldBuffer, vectorField.texture[in].image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                               &region);

        vectorField.texture[in].image.transitionLayout(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, DEFAULT_SUB_RANGE
                , VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT
                , VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    });
    vectorFieldSet = true;
}

void FluidSolver2D::createPipelines() {
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
                .addDescriptorSetLayouts({textureSetLayout, advectTextureSet, samplerSet})
                .addPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(constants))
            .renderPass(renderPass)
            .name("advect")
        .build(advectPipeline.layout);

    divergence.pipeline =
        builder
            .shaderStage()
                .fragmentShader(resource("divergence.frag.spv"))
            .layout().clear()
                .addDescriptorSetLayout( textureSetLayout)
                .addPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(constants))
            .renderPass(renderPass)
            .name("divergence")
        .build(divergence.layout);

    divergenceFree.pipeline =
        builder
            .shaderStage()
                .fragmentShader(resource("divergence_free_field.frag.spv"))
            .layout().clear()
                .addDescriptorSetLayouts({textureSetLayout, textureSetLayout})
                .addPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(constants))
            .renderPass(renderPass)
            .name("divergence_free_field")
        .build(divergenceFree.layout);

    jacobi.pipeline =
        builder
            .shaderStage()
                .fragmentShader(resource("jacobi.frag.spv"))
            .layout().clear()
                .addDescriptorSetLayouts({textureSetLayout, textureSetLayout})
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
            .name("sources")
        .build(addSourcePipeline.layout);

    vorticity.pipeline =
        builder
            .shaderStage()
                .fragmentShader(resource("vorticity.frag.spv"))
            .layout().clear()
                .addDescriptorSetLayout(textureSetLayout)
            .renderPass(renderPass)
            .name("vorticity")
        .build(vorticity.layout);

    vorticityForce.pipeline =
        builder
            .shaderStage()
                .fragmentShader(resource("vorticity_force.frag.spv"))
            .layout().clear()
                .addDescriptorSetLayouts({textureSetLayout, textureSetLayout})
                .addPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(vorticityForce.constants))
            .renderPass(renderPass)
            .name("vorticity_force")
        .build(vorticityForce.layout);

//    @formatter:on
}

std::string FluidSolver2D::resource(const std::string &name) {
    auto res = fileManager->getFullPath(name);
    assert(res.has_value());
    return res->string();
}

void FluidSolver2D::add(ExternalForce &&force) {
    externalForces.push_back(force);
}

void FluidSolver2D::runSimulation(VkCommandBuffer commandBuffer) {
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, screenQuad.vertices, &offset);
    velocityStep(commandBuffer);
    quantityStep(commandBuffer);
}

void FluidSolver2D::velocityStep(VkCommandBuffer commandBuffer) {
    if(!options.advectVField) return;

    clearForces(commandBuffer);
    applyForces(commandBuffer);
    if(options.viscosity > MIN_FLOAT) {
        jacobi.constants.isVectorField = 1;
        diffuse(commandBuffer, vectorField, options.viscosity);
        project(commandBuffer);
    }
    advectVectorField(commandBuffer);
    project(commandBuffer);
}

void FluidSolver2D::quantityStep(VkCommandBuffer commandBuffer) {
    for(auto& quantity : quantities){
        quantityStep(commandBuffer, quantity);
    }
}

void FluidSolver2D::quantityStep(VkCommandBuffer commandBuffer, Quantity &quantity) {
    clearSources(commandBuffer, quantity);
    updateSources(commandBuffer, quantity);
    addSource(commandBuffer, quantity);
    diffuseQuantity(commandBuffer, quantity);
    advectQuantity(commandBuffer, quantity);
}

void FluidSolver2D::clearSources(VkCommandBuffer commandBuffer, Quantity &quantity) {
    clear(commandBuffer, quantity.source.texture[in]);
    clear(commandBuffer, quantity.source.texture[out]);
}

void FluidSolver2D::updateSources(VkCommandBuffer commandBuffer, Quantity &quantity) {
    quantity.update(commandBuffer, quantity.source);
}

void FluidSolver2D::addSource(VkCommandBuffer commandBuffer, Quantity &quantity) {
    addSources(commandBuffer, quantity.source, quantity.field);
}

void FluidSolver2D::diffuseQuantity(VkCommandBuffer commandBuffer, Quantity &quantity) {
    jacobi.constants.isVectorField = 0;
    diffuse(commandBuffer, quantity.field, quantity.diffuseRate);
}

void FluidSolver2D::advectQuantity(VkCommandBuffer commandBuffer, Quantity &quantity) {
    static std::array<VkDescriptorSet, 2> sets;
    sets[0] = vectorField.descriptorSet[in];
    sets[1] = quantity.field.advectDescriptorSet[in];

    advect(commandBuffer, sets,quantity.field.framebuffer[out]);
    quantity.field.swap();
}

void FluidSolver2D::applyForces(VkCommandBuffer commandBuffer) {
    applyExternalForces(commandBuffer);
    computeVorticityConfinement(commandBuffer);
    addSources(commandBuffer, forceField, vectorField);
}

void FluidSolver2D::applyExternalForces(VkCommandBuffer commandBuffer) {
    for(const auto& externalForce : externalForces){
        withRenderPass(commandBuffer, forceField.framebuffer[out], [&](auto commandBuffer){
            externalForce(commandBuffer, forceField.descriptorSet[in]);
        });
        forceField.swap();
    }
}

void FluidSolver2D::computeVorticityConfinement(VkCommandBuffer commandBuffer) {
    if(!options.vorticity) return;
    withRenderPass(commandBuffer, vorticityField.framebuffer[0], [&](auto commandBuffer){
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vorticity.pipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vorticity.layout, 0, 1, &vectorField.descriptorSet[in], 0, VK_NULL_HANDLE);
        vkCmdDraw(commandBuffer, 4, 1, 0, 0);
    });
    withRenderPass(commandBuffer, forceField.framebuffer[out], [&](auto commandBuffer){
        static std::array<VkDescriptorSet, 2> sets;
        sets[0] = vorticityField.descriptorSet[in];
        sets[1] = forceField.descriptorSet[in];
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vorticityForce.pipeline);
        vkCmdPushConstants(commandBuffer, vorticityForce.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(vorticityForce.constants), &vorticityForce.constants);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vorticityForce.layout, 0, COUNT(sets), sets.data(), 0, VK_NULL_HANDLE);
        vkCmdDraw(commandBuffer, 4, 1, 0, 0);
    });
    forceField.swap();
}

void FluidSolver2D::clearForces(VkCommandBuffer commandBuffer) {
    clear(commandBuffer, forceField.texture[in]);
    clear(commandBuffer, forceField.texture[out]);
}

void FluidSolver2D::clear(VkCommandBuffer commandBuffer, Texture &texture) {
    texture.image.transitionLayout(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, DEFAULT_SUB_RANGE
            , VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT
            , VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

    VkClearColorValue color{ {0.f, 0.f, 0.f, 0.f}};
    VkImageSubresourceRange range{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    vkCmdClearColorImage(commandBuffer, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &color, 1, &range);

    texture.image.transitionLayout(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, DEFAULT_SUB_RANGE
            , VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT
            , VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
}

void FluidSolver2D::addSources(VkCommandBuffer commandBuffer, Field &sourceField, Field &destinationField) {
    addSourcePipeline.constants.dt = constants.dt;
    withRenderPass(commandBuffer, destinationField.framebuffer[out], [&](auto commandBuffer){
        static std::array<VkDescriptorSet, 2> sets;
        sets[0] = sourceField.descriptorSet[in];
        sets[1] = destinationField.descriptorSet[in];
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, addSourcePipeline.pipeline);
        vkCmdPushConstants(commandBuffer, addSourcePipeline.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(addSourcePipeline.constants), &addSourcePipeline.constants);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, addSourcePipeline.layout
                , 0, COUNT(sets), sets.data(), 0, VK_NULL_HANDLE);
        vkCmdDraw(commandBuffer, 4, 1, 0, 0);
    });
    destinationField.swap();
}

void FluidSolver2D::advectVectorField(VkCommandBuffer commandBuffer) {
    static std::array<VkDescriptorSet, 2> sets;
    sets[0] = vectorField.descriptorSet[in];
    sets[1] = vectorField.advectDescriptorSet[in];

    advect(commandBuffer, sets,vectorField.framebuffer[out]);
    vectorField.swap();
}

void FluidSolver2D::advect(VkCommandBuffer commandBuffer, const std::array<VkDescriptorSet, 2> &inSets,
                           VulkanFramebuffer &framebuffer) {


    static std::array<VkDescriptorSet, 3> sets;
    sets[0] = inSets[0];
    sets[1] = inSets[1];
    sets[2] = samplerDescriptorSet;
    withRenderPass(commandBuffer, framebuffer, [&](auto commandBuffer){
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, advectPipeline.pipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, advectPipeline.layout
                , 0, COUNT(sets), sets.data(), 0, VK_NULL_HANDLE);

        vkCmdPushConstants(commandBuffer, advectPipeline.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(constants), &constants);
        vkCmdDraw(commandBuffer, 4, 1, 0, 0);
    });
}

void FluidSolver2D::project(VkCommandBuffer commandBuffer) {
    if(!options.project) return;
    computeDivergence(commandBuffer);
    solvePressure(commandBuffer);
    computeDivergenceFreeField(commandBuffer);
    vectorField.swap();
}

void FluidSolver2D::computeDivergence(VkCommandBuffer commandBuffer) {
    withRenderPass(commandBuffer, divergenceField.framebuffer[0], [&](auto commandBuffer){
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, divergence.pipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, divergence.layout
                , 0, 1, &vectorField.descriptorSet[in], 0, VK_NULL_HANDLE);

        vkCmdPushConstants(commandBuffer, divergence.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(constants), &constants);
        vkCmdDraw(commandBuffer, 4, 1, 0, 0);
    });
}

void FluidSolver2D::solvePressure(VkCommandBuffer commandBuffer) {
    auto alpha = -constants.epsilon * constants.epsilon;
    auto rBeta = 0.25f;
    for(int i = 0; i < options.poissonIterations; i++){
        withRenderPass(commandBuffer, pressureField.framebuffer[out], [&](auto commandBuffer){
            jacobiIteration(commandBuffer, pressureField.descriptorSet[in], divergenceField.descriptorSet[in], alpha, rBeta);
            pressureField.swap();
        });
    }
}

void FluidSolver2D::jacobiIteration(VkCommandBuffer commandBuffer, VkDescriptorSet unknownDescriptor,
                                    VkDescriptorSet solutionDescriptor, float alpha, float rBeta) {
    jacobi.constants.alpha = alpha;
    jacobi.constants.rBeta = rBeta;
    static std::array<VkDescriptorSet, 2> sets;
    sets[0] = solutionDescriptor;
    sets[1] = unknownDescriptor;
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, jacobi.pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, jacobi.layout
            , 0, COUNT(sets), sets.data(), 0, VK_NULL_HANDLE);

    vkCmdPushConstants(commandBuffer, jacobi.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(jacobi.constants), &jacobi.constants);
    vkCmdDraw(commandBuffer, 4, 1, 0, 0);
}

void FluidSolver2D::computeDivergenceFreeField(VkCommandBuffer commandBuffer) {
    withRenderPass(commandBuffer, vectorField.framebuffer[out], [&](auto commandBuffer){
        static std::array<VkDescriptorSet, 2> sets;
        sets[0] = vectorField.descriptorSet[in];
        sets[1] = pressureField.descriptorSet[in];
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, divergenceFree.pipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, divergenceFree.layout
                , 0, COUNT(sets), sets.data(), 0, VK_NULL_HANDLE);

        vkCmdPushConstants(commandBuffer, divergenceFree.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(constants), &constants);
        vkCmdDraw(commandBuffer, 4, 1, 0, 0);
    });
}

void FluidSolver2D::withRenderPass(VkCommandBuffer commandBuffer, const VulkanFramebuffer &framebuffer
                                   , GpuProcess &&process, glm::vec4 clearColor) const {

    static std::array<VkClearValue, 1> clearValues;
    clearValues[0].color = {clearColor.b, clearColor.g, clearColor.b, clearColor.a};

    VkRenderPassBeginInfo rPassInfo = initializers::renderPassBeginInfo();
    rPassInfo.clearValueCount = COUNT(clearValues);
    rPassInfo.pClearValues = clearValues.data();
    rPassInfo.framebuffer = framebuffer;
    rPassInfo.renderArea.offset = {0u, 0u};
    rPassInfo.renderArea.extent = { static_cast<uint32_t>(gridSize.x), static_cast<uint32_t>(gridSize.y)};
    rPassInfo.renderPass = renderPass;

    vkCmdBeginRenderPass(commandBuffer, &rPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    process(commandBuffer);

    vkCmdEndRenderPass(commandBuffer);
}

void FluidSolver2D::diffuse(VkCommandBuffer commandBuffer, Field &field, float rate) {
    if(rate <= MIN_FLOAT) return;
    diffuseHelper.texture.image.transitionLayout(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, DEFAULT_SUB_RANGE
            , VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT
            , VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

    field.texture[in].image.transitionLayout(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, DEFAULT_SUB_RANGE
            , VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT
            , VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

    VkImageSubresourceLayers subresourceLayers{VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    VkExtent3D extent3D{ static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
    VkImageCopy pRegion{subresourceLayers, {0, 0, 0}, subresourceLayers, {0, 0, 0}, extent3D};

    vkCmdCopyImage(commandBuffer, field.texture[in].image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
            , diffuseHelper.texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &pRegion);

    field.texture[in].image.transitionLayout(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, DEFAULT_SUB_RANGE
            , VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT
            , VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

    diffuseHelper.texture.image.transitionLayout(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, DEFAULT_SUB_RANGE
            , VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT
            , VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);


    float alpha = (constants.epsilon * constants.epsilon)/(constants.dt * rate);
    float rBeta = 1.0f/(4.0f + alpha);
    for(int i = 0; i < options.poissonIterations; i++){
        withRenderPass(commandBuffer, field.framebuffer[out], [&](auto commandBuffer){
            jacobiIteration(commandBuffer, field.descriptorSet[in], diffuseHelper.solutionDescriptorSet, alpha, rBeta);
            field.swap();
        });
    }
}


void FluidSolver2D::renderVectorField(VkCommandBuffer commandBuffer) {
    if(!options.showArrows) return;
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, arrows.vertexBuffer, &offset);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, arrows.pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, arrows.layout
            , 0, 1, &vectorField.descriptorSet[in], 0
            , VK_NULL_HANDLE);

    vkCmdDraw(commandBuffer, arrows.numArrows, 1, 0, 0);
}

void FluidSolver2D::add(Quantity& quantity) {
    createFrameBuffer(quantity);
    createDescriptorSets(quantity);
    quantities.emplace_back(quantity);
}

void FluidSolver2D::createFrameBuffer(Quantity &quantity) {
    for(auto i = 0; i < 2; i++){
        quantity.field.framebuffer[i] = device->createFramebuffer(renderPass, {quantity.field.texture[i].imageView }, width, height);
        device->setName<VK_OBJECT_TYPE_FRAMEBUFFER>(fmt::format("{}_{}", quantity.name, i), quantity.field.framebuffer[i].frameBuffer);

        quantity.source.framebuffer[i] = device->createFramebuffer(renderPass, {quantity.source.texture[i].imageView }, width, height);
        device->setName<VK_OBJECT_TYPE_FRAMEBUFFER>(fmt::format("{}_source_{}", quantity.name, i), quantity.field.framebuffer[i].frameBuffer);
    }
}

void FluidSolver2D::dt(float value) {
    constants.dt = value;
}

float FluidSolver2D::dt() const {
    return constants.dt;
}

void FluidSolver2D::advectVelocity(bool flag) {
    options.advectVField = flag;
}

void FluidSolver2D::project(bool flag) {
    options.project = flag;
}

void FluidSolver2D::showVectors(bool flag) {
    options.showArrows = flag;
}

void FluidSolver2D::applyVorticity(bool flag) {
    options.vorticity = flag;
}

void FluidSolver2D::poissonIterations(int value) {
    options.poissonIterations = value;
}

void FluidSolver2D::viscosity(float value) {
    options.viscosity = value;
}
