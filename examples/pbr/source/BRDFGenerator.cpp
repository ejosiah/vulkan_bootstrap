#include "BRDFGenerator.hpp"
#include "GraphicsPipelineBuilder.hpp"

BRDFGenerator::BRDFGenerator(VulkanDevice *device, FileManager* fileManager)
: device{ device }
, fileManager{ fileManager}
{
    
}

void BRDFGenerator::init() {
        createFramebufferAttachment();
        createRenderPass();
        createFramebuffer();
        createScreenVertices();
        createPipeline();
}

void BRDFGenerator::createRenderPass() {
    std::vector<VkAttachmentDescription> attachments{
            {
                0,
                VK_FORMAT_R32G32B32A32_SFLOAT,
                VK_SAMPLE_COUNT_1_BIT,
                VK_ATTACHMENT_LOAD_OP_CLEAR,
                VK_ATTACHMENT_STORE_OP_STORE,
                VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                VK_ATTACHMENT_STORE_OP_DONT_CARE,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            }
    };
    std::vector<SubpassDescription> subpasses{
            {
                0,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                {},
                {{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}},
                {},
                {},
                {}
            }
    };
    std::vector<VkSubpassDependency> dependencies{
            {
                VK_SUBPASS_EXTERNAL,
                0,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                0,
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                0
            }
    };
    renderPass = device->createRenderPass(attachments, subpasses, dependencies);
}

void BRDFGenerator::createFramebufferAttachment() {
    VkImageCreateInfo createInfo = initializers::imageCreateInfo(
            VK_IMAGE_TYPE_2D,
            VK_FORMAT_R32G32B32A32_SFLOAT,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            512, 512);
    colorAttachment.image = device->createImage(createInfo);

    VkImageSubresourceRange range = initializers::imageSubresourceRange();
    colorAttachment.imageView = colorAttachment.image.createView(VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_VIEW_TYPE_2D, range);
}

void BRDFGenerator::createFramebuffer() {
    std::vector<VkImageView> attachments{ colorAttachment.imageView };
    framebuffer = device->createFramebuffer(renderPass, attachments, 512, 512);
}

void BRDFGenerator::createPipeline() {
    pipeline =
        device->graphicsPipelineBuilder()
            .shaderStage()
                .vertexShader(fileManager->load("brdf.vert.spv"))
                .fragmentShader(fileManager->load("brdf.frag.spv"))
            .vertexInputState()
                .addVertexBindingDescriptions(ClipSpace::bindingDescription())
                .addVertexAttributeDescriptions(ClipSpace::attributeDescriptions())
            .inputAssemblyState()
                .triangleStrip()
            .viewportState()
                .viewport()
                    .origin(0, 0)
                    .dimension({512u, 512u})
                    .minDepth(0)
                    .maxDepth(1)
                .scissor()
                    .offset(0, 0)
                    .extent({512u, 512u})
                .add()
            .rasterizationState()
                .cullBackFace()
                .frontFaceCounterClockwise()
                .polygonModeFill()
            .multisampleState() 
                .rasterizationSamples(VK_SAMPLE_COUNT_1_BIT)
            .colorBlendState()
                .attachment().add()
            .renderPass(renderPass)
            .subpass(0)
            .name("brdf_generator")
        .build(layout);
            
}

Texture BRDFGenerator::generate() {
    Texture texture;
    
    device->graphicsCommandPool().oneTimeCommand([&](auto commandBuffer){

        VkClearValue clearValue{};
        clearValue.color = {0, 0, 0, 0};
        VkRenderPassBeginInfo rPassInfo = initializers::renderPassBeginInfo();
        rPassInfo.clearValueCount = 1;
        rPassInfo.pClearValues = &clearValue;
        rPassInfo.framebuffer = framebuffer;
        rPassInfo.renderArea.offset = {0u, 0u};
        rPassInfo.renderArea.extent = { 512u, 512u};
        rPassInfo.renderPass = renderPass;

        VkDeviceSize offset = 0;
        vkCmdBeginRenderPass(commandBuffer, &rPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, screenVertices, &offset);
        vkCmdDraw(commandBuffer, 4, 1, 0, 0);
        vkCmdEndRenderPass(commandBuffer);
    });
    
    texture.image = std::move(colorAttachment.image);
    texture.imageView = std::move(colorAttachment.imageView);
    
    return texture;
}

void BRDFGenerator::createScreenVertices() {
    auto quad = ClipSpace::Quad::positions;
    screenVertices = device->createDeviceLocalBuffer(quad.data(), BYTE_SIZE(quad), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}
