#include "FluidSimulation.hpp"
#include "GraphicsPipelineBuilder.hpp"
#include "DescriptorSetBuilder.hpp"
#include "vulkan_image_ops.h"

FluidSimulation::FluidSimulation(const Settings& settings) : VulkanBaseApp("Fluid Simulation", settings) {
    fileManager.addSearchPath(".");
    fileManager.addSearchPath("../../examples/fluid_simulation");
    fileManager.addSearchPath("../../examples/fluid_simulation/spv");
    fileManager.addSearchPath("../../examples/fluid_simulation/models");
    fileManager.addSearchPath("../../examples/fluid_simulation/textures");
    fileManager.addSearchPath("../../data/shaders");
    fileManager.addSearchPath("../../data/models");
    fileManager.addSearchPath("../../data/textures");
    fileManager.addSearchPath("../../data");
    constants.epsilon = 1.0f/float(width);    // TODO 2d epsilon
}

void FluidSimulation::initApp() {
    createSamplers();
    initSimulationStages();
    initColorField();
    initFullScreenQuad();
    createDescriptorPool();
    createCommandPool();
    createDescriptorSetLayouts();
    updateDescriptorSets();

    createPipelineCache();
    createRenderPipeline();
    initFluidSolver();
    debugBuffer = device.createBuffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_TO_CPU, sizeof(glm::vec4) * width * height, "debug");
}


void FluidSimulation::initFluidSolver() {

    float dx = 1.0f/float(width);
    std::vector<glm::vec4> field;
    float maxLength = MIN_FLOAT;
    constexpr auto two_pi = glm::two_pi<float>();
    for(int i = 0; i < height; i++){
        for(int j = 0; j < width; j++){
            auto x = 2 * float(j)/float(width) - 1;
            auto y = 2 * float(i)/float(height) - 1;
//            glm::vec2 u{glm::sin(two_pi * y), glm::sin(two_pi * x)};
//            glm::vec2 u{1, glm::sin(two_pi * x)};
//            glm::vec2 u{x, y}; // divergent fields 1;
//            glm::vec2 u{glm::sin(two_pi * x), 0}; // divergent fields 2;
            glm::vec2 u{y, x}; // divergent fields 3;
            maxLength = glm::max(glm::length(u), maxLength);
            field.emplace_back(u , 0, 0);
        }
    }

    auto stagingBuffer = device.createStagingBuffer(BYTE_SIZE(field));
    stagingBuffer.copy(field);

    fluidSolver = FluidSolver2D{&device, &descriptorPool, &renderPass, &fileManager, {width, height}};
    fluidSolver.init();
    fluidSolver.set(stagingBuffer);
    fluidSolver.dt((5.0f * dx)/maxLength);
    fluidSolver.add(colorQuantity);

}

void FluidSimulation::initColorField() {

    auto checkerboard = [](int i, int j, float w, float h){
        auto x = 2 * (float(j)/w) - 1;
        auto y = 2 * (float(i)/h) - 1;
        return glm::vec3{
                glm::step(1.0, glm::mod(floor((x + 1.0) / 0.2) + floor((y + 1.0) / 0.2), 2.0)),
                glm::step(1.0, glm::mod(floor((x + 1.0) / 0.2) + floor((y + 1.0) / 0.2), 2.0)),
                glm::step(1.0, glm::mod(floor((x + 1.0) / 0.2) + floor((y + 1.0) / 0.2), 2.0))
        };
    };
    std::vector<glm::vec4> field;
    std::vector<glm::vec4> allocation(width * height);
    for(auto i = 0; i < height; i++){
        for(auto j = 0; j < width; j++){
            auto color = checkerboard(j, i, float(width), float(height));
            field.emplace_back(color, 1);
        }
    }
//    field = allocation;

    textures::create(device, colorQuantity.field.texture[0], VK_IMAGE_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT
            , field.data(), {width, height, 1}, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
            , sizeof(float));
    textures::create(device, colorQuantity.field.texture[1], VK_IMAGE_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT
            , field.data(), {width, height, 1}, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
            , sizeof(float));


    textures::create(device, colorQuantity.source.texture[0], VK_IMAGE_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT
            , allocation.data(), {width, height, 1}, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
            , sizeof(float));
    textures::create(device, colorQuantity.source.texture[1], VK_IMAGE_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT
            , allocation.data(), {width, height, 1}, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
            , sizeof(float));

    device.setName<VK_OBJECT_TYPE_IMAGE>(fmt::format("{}_{}", "color_field", 0), colorQuantity.field.texture[0].image.image);
    device.setName<VK_OBJECT_TYPE_IMAGE>(fmt::format("{}_{}", "color_field", 1), colorQuantity.field.texture[1].image.image);

    device.setName<VK_OBJECT_TYPE_IMAGE_VIEW>(fmt::format("{}_{}", "color_field", 0), colorQuantity.field.texture[0].imageView.handle);
    device.setName<VK_OBJECT_TYPE_IMAGE_VIEW>(fmt::format("{}_{}", "color_field", 1), colorQuantity.field.texture[1].imageView.handle);

    for(auto i = 0; i < 2; i++){
        colorQuantity.field.framebuffer[i] = device.createFramebuffer(simRenderPass, { colorQuantity.field.texture[i].imageView }, width, height);
        device.setName<VK_OBJECT_TYPE_FRAMEBUFFER>(fmt::format("{}_{}", "color_field", i), colorQuantity.field.framebuffer[i].frameBuffer);

        colorQuantity.source.framebuffer[i] = device.createFramebuffer(simRenderPass, { colorQuantity.source.texture[i].imageView }, width, height);
        device.setName<VK_OBJECT_TYPE_FRAMEBUFFER>(fmt::format("{}_{}", "color_source_field", i), colorQuantity.field.framebuffer[i].frameBuffer);
    }
    colorQuantity.update = [&](VkCommandBuffer commandBuffer, VulkanRenderPass& renderPass, Field& field){
        addDyeSource(commandBuffer, renderPass, field, {0.004, 0, 0}, {0.2, 0.2});
        addDyeSource(commandBuffer, renderPass, field, {0, 0, 0.004}, {0.5, 0.9});
        addDyeSource(commandBuffer, renderPass, field,  {0, 0.004, 0}, {0.8, 0.2});
    };
    fluidSolver.add(colorQuantity);
}

void FluidSimulation::initFullScreenQuad() {
    auto quad = ClipSpace::Quad::positions;
    screenQuad.vertices = device.createDeviceLocalBuffer(quad.data(), BYTE_SIZE(quad), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

void FluidSimulation::createDescriptorPool() {
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

void FluidSimulation::createDescriptorSetLayouts() {
    textureSetLayout =
        device.descriptorSetLayoutBuilder()
            .name("texture_set")
            .binding(0)
                .descriptorType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                .descriptorCount(1)
                .shaderStages(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
            .createLayout();

    auto sets = descriptorPool.allocate(
            {
                textureSetLayout, textureSetLayout, textureSetLayout, textureSetLayout
                , textureSetLayout, textureSetLayout, textureSetLayout, textureSetLayout
                , textureSetLayout, textureSetLayout, textureSetLayout
            });

;
    colorQuantity.field.descriptorSet[0] = sets[0];
    colorQuantity.field.descriptorSet[1] = sets[1];
    divergenceField.descriptorSet[0] = sets[2];
    pressureField.descriptorSet[0] = sets[3];
    pressureField.descriptorSet[1] = sets[4];
    diffuseHelper.solutionDescriptorSet = sets[5];
    forceField.descriptorSet[0] = sets[6];
    forceField.descriptorSet[1] = sets[7];
    vorticityField.descriptorSet[0] = sets[8];
    colorQuantity.source.descriptorSet[0] = sets[9];
    colorQuantity.source.descriptorSet[1] = sets[10];


    device.setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>(fmt::format("{}_{}", "color_field", 0), colorQuantity.field.descriptorSet[0]);
    device.setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>(fmt::format("{}_{}", "color_field", 1), colorQuantity.field.descriptorSet[1]);

    device.setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>("divergence_field", divergenceField.descriptorSet[0]);

    device.setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>(fmt::format("{}_{}", "pressure_field", 0), pressureField.descriptorSet[0]);
    device.setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>(fmt::format("{}_{}", "pressure_field", 1), pressureField.descriptorSet[1]);

    device.setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>("diffuse_solution_container", diffuseHelper.solutionDescriptorSet);

    device.setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>(fmt::format("{}_{}", "force_field", 0), forceField.descriptorSet[0]);
    device.setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>(fmt::format("{}_{}", "force_field", 1), forceField.descriptorSet[1]);

    device.setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>("vorticity_field", vorticityField.descriptorSet[0]);

    device.setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>(fmt::format("{}_{}", "color_source_field", 0), colorQuantity.source.descriptorSet[0]);
    device.setName<VK_OBJECT_TYPE_DESCRIPTOR_SET>(fmt::format("{}_{}", "color_source_field", 1), colorQuantity.source.descriptorSet[1]);
}

void FluidSimulation::updateDescriptorSets() {
    auto writes = initializers::writeDescriptorSets<11>();


    writes[0].dstSet = colorQuantity.field.descriptorSet[0];
    writes[0].dstBinding = 0;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[0].descriptorCount = 1;
    VkDescriptorImageInfo c0Info{colorQuantity.field.texture[0].sampler
                                 , colorQuantity.field.texture[0].imageView
                                 , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    writes[0].pImageInfo = &c0Info;

    writes[1].dstSet = colorQuantity.field.descriptorSet[1];
    writes[1].dstBinding = 0;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[1].descriptorCount = 1;
    VkDescriptorImageInfo c1Info{colorQuantity.field.texture[1].sampler
                                 , colorQuantity.field.texture[1].imageView
                                 , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    writes[1].pImageInfo = &c1Info;

    writes[2].dstSet = divergenceField.descriptorSet[0];
    writes[2].dstBinding = 0;
    writes[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[2].descriptorCount = 1;
    VkDescriptorImageInfo divInfo{divergenceField.texture[0].sampler
            , divergenceField.texture[0].imageView
            , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    writes[2].pImageInfo = &divInfo;

    writes[3].dstSet = pressureField.descriptorSet[0];
    writes[3].dstBinding = 0;
    writes[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[3].descriptorCount = 1;
    VkDescriptorImageInfo p0Info{pressureField.texture[0].sampler
            , pressureField.texture[0].imageView
            , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    writes[3].pImageInfo = &p0Info;

    writes[4].dstSet = pressureField.descriptorSet[1];
    writes[4].dstBinding = 0;
    writes[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[4].descriptorCount = 1;
    VkDescriptorImageInfo p1Info{pressureField.texture[1].sampler
            , pressureField.texture[1].imageView
            , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    writes[4].pImageInfo = &p1Info;

    writes[5].dstSet = diffuseHelper.solutionDescriptorSet;
    writes[5].dstBinding = 0;
    writes[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[5].descriptorCount = 1;
    VkDescriptorImageInfo diffuseInfo{diffuseHelper.texture.sampler, diffuseHelper.texture.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    writes[5].pImageInfo = &diffuseInfo;

    writes[6].dstSet = forceField.descriptorSet[0];
    writes[6].dstBinding = 0;
    writes[6].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[6].descriptorCount = 1;
    VkDescriptorImageInfo forceInfo{forceField.texture[0].sampler
            , forceField.texture[0].imageView
            , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    writes[6].pImageInfo = &forceInfo;

    writes[7].dstSet = forceField.descriptorSet[1];
    writes[7].dstBinding = 0;
    writes[7].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[7].descriptorCount = 1;
    VkDescriptorImageInfo force1Info{forceField.texture[1].sampler
            , forceField.texture[1].imageView
            , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    writes[7].pImageInfo = &force1Info;

    writes[8].dstSet = vorticityField.descriptorSet[0];
    writes[8].dstBinding = 0;
    writes[8].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[8].descriptorCount = 1;
    VkDescriptorImageInfo vorticityInfo{vorticityField.texture[0].sampler
            , vorticityField.texture[0].imageView
            , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    writes[8].pImageInfo = &vorticityInfo;

    writes[9].dstSet = colorQuantity.source.descriptorSet[0];
    writes[9].dstBinding = 0;
    writes[9].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[9].descriptorCount = 1;
    VkDescriptorImageInfo csInfo{colorQuantity.source.texture[0].sampler
            , colorQuantity.source.texture[0].imageView
            , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    writes[9].pImageInfo = &csInfo;

    writes[10].dstSet = colorQuantity.source.descriptorSet[1];
    writes[10].dstBinding = 0;
    writes[10].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[10].descriptorCount = 1;
    VkDescriptorImageInfo cs1Info{colorQuantity.source.texture[1].sampler
            , colorQuantity.source.texture[1].imageView
            , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    writes[10].pImageInfo = &cs1Info;

    device.updateDescriptorSets(writes);
}

void FluidSimulation::createCommandPool() {
    commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandBuffers = commandPool.allocateCommandBuffers(swapChainImageCount);
}

void FluidSimulation::createPipelineCache() {
    pipelineCache = device.createPipelineCache();
}


void FluidSimulation::createRenderPipeline() {
    //    @formatter:off
    auto builder = device.graphicsPipelineBuilder();
    render.pipeline =
        builder
            .allowDerivatives()
            .shaderStage()
                .vertexShader(resource("pass_through.vert.spv"))
                .fragmentShader(resource("pass_through.frag.spv"))
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
                .subpass(0)
                .name("render")
                .pipelineCache(pipelineCache)
            .build(render.layout);

    arrows.pipeline =
        builder
            .basePipeline(render.pipeline)
            .shaderStage()
                .vertexShader(resource("vectorField.vert.spv"))
                .fragmentShader(resource("vectorField.frag.spv"))
            .vertexInputState().clear()
                .addVertexBindingDescription(0, sizeof(Vector), VK_VERTEX_INPUT_RATE_VERTEX)
                .addVertexAttributeDescription(0, 0, VK_FORMAT_R32G32_SFLOAT, offsetOf(Vector, vertex))
                .addVertexAttributeDescription(1, 0, VK_FORMAT_R32G32_SFLOAT, offsetOf(Vector, position))
            .layout()
                .addDescriptorSetLayout(textureSetLayout)
            .name("vector_field")
        .build(arrows.layout);

    screenQuad.pipeline =
        builder
            .basePipeline(render.pipeline)
            .shaderStage()
                .vertexShader(resource("quad.vert.spv"))
                .fragmentShader(resource("quad.frag.spv"))
            .vertexInputState().clear()
                .addVertexBindingDescriptions(ClipSpace::bindingDescription())
                .addVertexAttributeDescriptions(ClipSpace::attributeDescriptions())
            .inputAssemblyState()
                .triangleStrip()
            .layout()
                .addDescriptorSetLayout(textureSetLayout)
            .name("fullscreen_quad")
        .build(screenQuad.layout);

    advectPipeline.pipeline =
        builder
            .basePipeline(render.pipeline)
            .shaderStage()
                .vertexShader(resource("quad.vert.spv"))
                .fragmentShader(resource("advect.frag.spv"))
            .layout().clear()
                .addDescriptorSetLayouts({textureSetLayout, textureSetLayout})
                .addPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(constants))
            .renderPass(simRenderPass)
            .name("advect")
        .build(advectPipeline.layout);

    divergence.pipeline =
        builder
            .shaderStage()
                .fragmentShader(resource("divergence.frag.spv"))
            .layout().clear()
                .addDescriptorSetLayout( textureSetLayout)
                .addPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(constants))
            .renderPass(simRenderPass)
            .name("divergence")
        .build(divergence.layout);

    pressure.pipeline =
        builder
            .shaderStage()
                .fragmentShader(resource("pressure_solver.frag.spv"))
            .layout().clear()
                .addDescriptorSetLayouts({textureSetLayout, textureSetLayout})
                .addPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(constants))
            .renderPass(simRenderPass)
            .name("pressure_solver")
        .build(pressure.layout);

    divergenceFree.pipeline =
        builder
            .shaderStage()
                .fragmentShader(resource("divergence_free_field.frag.spv"))
            .layout().clear()
                .addDescriptorSetLayouts({textureSetLayout, textureSetLayout})
                .addPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(constants))
            .renderPass(simRenderPass)
            .name("divergence_free_field")
        .build(divergenceFree.layout);

    jacobi.pipeline =
        builder
            .shaderStage()
                .fragmentShader(resource("jacobi.frag.spv"))
            .layout().clear()
                .addDescriptorSetLayouts({textureSetLayout, textureSetLayout})
                .addPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(jacobi.constants))
            .renderPass(simRenderPass)
            .name("jacobi")
        .build(jacobi.layout);

    addSource.pipeline =
        builder
            .shaderStage()
                .fragmentShader(resource("add_sources.frag.spv"))
            .layout().clear()
                .addDescriptorSetLayouts({textureSetLayout, textureSetLayout})
                .addPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(addSource.constants))
            .renderPass(simRenderPass)
            .name("sources")
        .build(addSource.layout);

    forceGen.pipeline =
        builder
            .shaderStage()
                .fragmentShader(resource("force.frag.spv"))
            .layout().clear()
                .addDescriptorSetLayouts({textureSetLayout})
                .addPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(forceGen.constants))
            .renderPass(simRenderPass)
            .name("force_generator")
        .build(forceGen.layout);

    vorticity.pipeline =
        builder
            .shaderStage()
                .fragmentShader(resource("vorticity.frag.spv"))
            .layout().clear()
                .addDescriptorSetLayout(textureSetLayout)
            .renderPass(simRenderPass)
            .name("vorticity")
        .build(vorticity.layout);

    vorticityForce.pipeline =
        builder
            .shaderStage()
                .fragmentShader(resource("vorticity_force.frag.spv"))
            .layout().clear()
                .addDescriptorSetLayouts({textureSetLayout, textureSetLayout})
                .addPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(vorticityForce.constants))
            .renderPass(simRenderPass)
            .name("vorticity_force")
        .build(vorticityForce.layout);

    dyeSource.pipeline =
        builder
            .shaderStage()
                .fragmentShader(resource("color_source.frag.spv"))
            .layout().clear()
                .addDescriptorSetLayouts({textureSetLayout})
                .addPushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(dyeSource.constants))
            .renderPass(simRenderPass)
            .name("color_source")
        .build(dyeSource.layout);

    //    @formatter:on
}

void FluidSimulation::initSimulationStages() {
    initProjectionStage();
}

void FluidSimulation::initProjectionStage() {
    initStage(simRenderPass, "fluid_simulation");

    std::vector<glm::vec4> allocation( width * height);

    textures::create(device, divergenceField.texture[0], VK_IMAGE_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT, allocation.data(), {width, height, 1}, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
                     , sizeof(float));
    textures::create(device, pressureField.texture[0], VK_IMAGE_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT, allocation.data(), {width, height, 1}, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
                     , sizeof(float));
    textures::create(device, pressureField.texture[1], VK_IMAGE_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT, allocation.data(), {width, height, 1}, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
                     , sizeof(float));

    textures::create(device, forceField.texture[0], VK_IMAGE_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT, allocation.data(), {width, height, 1}, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
                     , sizeof(float));
    textures::create(device, forceField.texture[1], VK_IMAGE_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT, allocation.data(), {width, height, 1}, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
                     , sizeof(float));

    textures::create(device, vorticityField.texture[0], VK_IMAGE_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT, allocation.data(), {width, height, 1}, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
                     , sizeof(float));

    textures::create(device, diffuseHelper.texture, VK_IMAGE_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT, allocation.data(), {width, height, 1}, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
                     , sizeof(float));

    divergenceField.framebuffer[0] = device.createFramebuffer(simRenderPass, { divergenceField.texture[0].imageView }, width, height);
    device.setName<VK_OBJECT_TYPE_FRAMEBUFFER>("divergence_field", divergenceField.framebuffer[0].frameBuffer);

    forceField.framebuffer[0] = device.createFramebuffer(simRenderPass, { forceField.texture[0].imageView }, width, height);
    forceField.framebuffer[1] = device.createFramebuffer(simRenderPass, { forceField.texture[1].imageView }, width, height);
    device.setName<VK_OBJECT_TYPE_FRAMEBUFFER>(fmt::format("{}_{}", "force_field", 0), forceField.framebuffer[0].frameBuffer);
    device.setName<VK_OBJECT_TYPE_FRAMEBUFFER>(fmt::format("{}_{}", "force_field", 1), forceField.framebuffer[1].frameBuffer);

    vorticityField.framebuffer[0] = device.createFramebuffer(simRenderPass, { vorticityField.texture[0].imageView }, width, height);
    device.setName<VK_OBJECT_TYPE_FRAMEBUFFER>("vorticity_field", vorticityField.framebuffer[0].frameBuffer);

    for(auto i = 0; i < 2; i++){
        pressureField.framebuffer[i] = device.createFramebuffer(simRenderPass, { pressureField.texture[i].imageView }, width, height);
        device.setName<VK_OBJECT_TYPE_FRAMEBUFFER>(fmt::format("{}_{}", "pressure_field", i), pressureField.framebuffer[i].frameBuffer);
    }
}

void FluidSimulation::initStage(VulkanRenderPass &renderPass, const std::string &name) {
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

    renderPass = device.createRenderPass({attachmentDescription}, subpasses, dependencies);
    device.setName<VK_OBJECT_TYPE_RENDER_PASS>(name, renderPass.renderPass);
}


void FluidSimulation::onSwapChainDispose() {
    dispose(render.pipeline);
    dispose(compute.pipeline);
}

void FluidSimulation::onSwapChainRecreation() {
    initFluidSolver();
    updateDescriptorSets();
    createRenderPipeline();
}

VkCommandBuffer *FluidSimulation::buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) {
    numCommandBuffers = 1;
    auto& commandBuffer = commandBuffers[imageIndex];

    VkCommandBufferBeginInfo beginInfo = initializers::commandBufferBeginInfo();
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    static std::array<VkClearValue, 2> clearValues;
    clearValues[0].color = {1, 1, 1, 1};
    clearValues[1].depthStencil = {1.0, 0u};

    VkRenderPassBeginInfo rPassInfo = initializers::renderPassBeginInfo();
    rPassInfo.clearValueCount = COUNT(clearValues);
    rPassInfo.pClearValues = clearValues.data();
    rPassInfo.framebuffer = framebuffers[imageIndex];
    rPassInfo.renderArea.offset = {0u, 0u};
    rPassInfo.renderArea.extent = swapChain.extent;
    rPassInfo.renderPass = renderPass;

    vkCmdBeginRenderPass(commandBuffer, &rPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    fluidSolver.renderVectorField(commandBuffer);
//    renderVectorField(commandBuffer);
    renderColorField(commandBuffer);

    vkCmdEndRenderPass(commandBuffer);
    vkEndCommandBuffer(commandBuffer);

    return &commandBuffer;
}

void FluidSimulation::renderColorField(VkCommandBuffer commandBuffer) {
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, screenQuad.vertices, &offset);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, screenQuad.pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, screenQuad.layout
            , 0, 1, &colorQuantity.field.descriptorSet[in], 0
            , VK_NULL_HANDLE);

    vkCmdDraw(commandBuffer, 4, 1, 0, 0);
}

void FluidSimulation::update(float time) {
    auto title = fmt::format("{} - fps {}", this->title, framePerSecond);
    glfwSetWindowTitle(window, title.c_str());
    runSimulation();
}

void FluidSimulation::runSimulation() {
    device.graphicsCommandPool().oneTimeCommand([&](auto commandBuffer){
//        VkDeviceSize offset = 0;
//        vkCmdBindVertexBuffers(commandBuffer, 0, 1, screenQuad.vertices, &offset);
//        if(options.advectVField) {
//            clearForces(commandBuffer);
//            applyForces(commandBuffer);
//            if(options.viscosity <= MIN_FLOAT) {
//                diffuse(commandBuffer, vectorField, options.viscosity);
//                project(commandBuffer);
//            }
//            advectVectorField(commandBuffer);
//            project(commandBuffer);
//        }
//        clearSources(commandBuffer);
//        addColors(commandBuffer);
//        diffuse(commandBuffer, colorQuantity.field, options.diffuseRate);
//        advectColor(commandBuffer);
        fluidSolver.runSimulation(commandBuffer);
    });

}

void FluidSimulation::userInputForce(VkCommandBuffer commandBuffer) {
    withRenderPass(commandBuffer, simRenderPass, forceField.framebuffer[out], [&](auto commandBuffer){
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, forceGen.pipeline);
        vkCmdPushConstants(commandBuffer, forceGen.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(forceGen.constants), &forceGen.constants);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, forceGen.layout, 0, 1, &forceField.descriptorSet[in], 0, VK_NULL_HANDLE);
        vkCmdDraw(commandBuffer, 4, 1, 0, 0);
    });
    forceField.swap();
    forceGen.constants.force.x = 0;
    forceGen.constants.force.y = 0;
}

void FluidSimulation::addDyeSource(VkCommandBuffer commandBuffer, VulkanRenderPass &renderPass, Field &field,
                                   glm::vec3 color, glm::vec2 source) {

    dyeSource.constants.dt = constants.dt;
    dyeSource.constants.color.rgb = color;
    dyeSource.constants.source = source;
    withRenderPass(commandBuffer, renderPass, field.framebuffer[out], [&](auto commandBuffer){
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, dyeSource.pipeline);
        vkCmdPushConstants(commandBuffer, dyeSource.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(dyeSource.constants), &dyeSource.constants);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, dyeSource.layout, 0, 1, &field.descriptorSet[in], 0, VK_NULL_HANDLE);
        vkCmdDraw(commandBuffer, 4, 1, 0, 0);
    });
    field.swap();
}



void FluidSimulation::withRenderPass(VkCommandBuffer commandBuffer, const VulkanRenderPass& renderPass
                                     , const VulkanFramebuffer &framebuffer, GpuProcess &&process, glm::vec4 clearColor) {
    static std::array<VkClearValue, 1> clearValues;
    clearValues[0].color = {clearColor.b, clearColor.g, clearColor.b, clearColor.a};

    VkRenderPassBeginInfo rPassInfo = initializers::renderPassBeginInfo();
    rPassInfo.clearValueCount = COUNT(clearValues);
    rPassInfo.pClearValues = clearValues.data();
    rPassInfo.framebuffer = framebuffer;
    rPassInfo.renderArea.offset = {0u, 0u};
    rPassInfo.renderArea.extent = swapChain.extent;
    rPassInfo.renderPass = renderPass;

    vkCmdBeginRenderPass(commandBuffer, &rPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    process(commandBuffer);

    vkCmdEndRenderPass(commandBuffer);
}

void FluidSimulation::checkAppInputs() {
    static bool initialPress = true;
    if(mouse.left.held){
        if(initialPress){
            initialPress = false;
            auto mousePos = glm::vec3(mouse.position, 1);
            glm::vec4 viewport{0, 0, swapChain.width(), swapChain.height()};
            forceGen.constants.center =  glm::unProject(mousePos, glm::mat4(1), glm::mat4(1), viewport).xy();
            spdlog::info("center: {}", forceGen.constants.center);
        }
    }else if(mouse.left.released){
        initialPress = true;
        auto mousePos = glm::vec3(mouse.position, 1);
        glm::vec4 viewport{0, 0, swapChain.width(), swapChain.height()};
        auto pos =  glm::unProject(mousePos, glm::mat4(1), glm::mat4(1), viewport).xy();

        auto displacement = pos - forceGen.constants.center;
        forceGen.constants.force = displacement;
        forceGen.constants.center = .5f * forceGen.constants.center + .5f;
        spdlog::info("pos: {}, center: {}, radius: {}, force: {}", pos, forceGen.constants.center, forceGen.constants.radius, forceGen.constants.force);
    }
}

void FluidSimulation::cleanup() {
    // TODO save pipeline cache
    VulkanBaseApp::cleanup();
}

void FluidSimulation::onPause() {
    VulkanBaseApp::onPause();
}

void FluidSimulation::createSamplers() {
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST ;

    valueSampler = device.createSampler(samplerInfo);

    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    linearSampler = device.createSampler(samplerInfo);
}

int main(){
    try{

        Settings settings;
        settings.width = 600;
        settings.height = 600;
        settings.depthTest = true;
        settings.vSync = true;
//        spdlog::set_level(spdlog::level::err);
        auto app = FluidSimulation{settings };
        app.run();
    }catch(std::runtime_error& err){
        spdlog::error(err.what());
    }
}