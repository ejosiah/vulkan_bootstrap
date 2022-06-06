#include "CSOView.hpp"
#include "GraphicsPipelineBuilder.hpp"


CSOView::CSOView(uint32_t w, uint32_t h, VulkanDevice *device, InputManager* inputMgr, FileManager *fileMgr)
: width{ w }
, height{ h }
, device{ device }
, inputManager{ inputMgr }
, fileManager{ fileMgr }
{
    createBuffers();
    initCamera();
    createRenderTarget();
    createRenderPass();
    createFrameBuffer();
    createPipeline();
}

void CSOView::createRenderTarget() {
    auto usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    auto imageInfo = initializers::imageCreateInfo(VK_IMAGE_TYPE_2D, format, usage, width, height);
    renderTarget.image = device->createImage(imageInfo);
    device->graphicsCommandPool().oneTimeCommand([&](auto commandBuffer){
        renderTarget.image.transitionLayout(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, DEFAULT_SUB_RANGE
                , 0, VK_ACCESS_SHADER_READ_BIT
                ,VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    });

    auto subRange = DEFAULT_SUB_RANGE;
    renderTarget.imageView = renderTarget.image.createView(format, VK_IMAGE_VIEW_TYPE_2D, subRange);
}

void CSOView::createRenderPass() {

    std::vector<VkAttachmentDescription> attachments{
            {
                0,
                format,
                VK_SAMPLE_COUNT_1_BIT,
                VK_ATTACHMENT_LOAD_OP_CLEAR,
                VK_ATTACHMENT_STORE_OP_STORE,
                VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                VK_ATTACHMENT_STORE_OP_DONT_CARE,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            }
    };
    std::vector<SubpassDescription> subpasses(1);
    subpasses[0].colorAttachments.push_back({0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});

    std::vector<VkSubpassDependency> dependencies(1);
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[0].dstStageMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    renderPass = device->createRenderPass(attachments, subpasses, dependencies);
}

void CSOView::createFrameBuffer() {
    std::vector<VkImageView> attachments{ renderTarget.imageView };
    framebuffer = device->createFramebuffer(renderPass, attachments, width, height);
}

void CSOView::render() {
    device->graphicsCommandPool().oneTimeCommand([&](auto commandBuffer){

        VkClearValue clearValue{ 0.0f, 0.0f, 0.0f, 1.0f };
        VkRenderPassBeginInfo renderPassInfo = initializers::renderPassBeginInfo();
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = framebuffer;
        renderPassInfo.renderArea = { {0, 0}, {width, height}};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearValue;
        
        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkDeviceSize offset = 0;

        if(simplexSize > 2) {
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, cso.pipeline);
            cameraController->push(commandBuffer, cso.layout);
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, csoBuffer, &offset);
            vkCmdBindIndexBuffer(commandBuffer, csoIndexBuffer, 0, VK_INDEX_TYPE_UINT16);
            uint32_t indexCount = simplexSize;
            vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);

        }


        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, origin.pipeline);
        cameraController->push(commandBuffer, origin.layout);
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, originBuffer, &offset);
        vkCmdDraw(commandBuffer, 1, 1, 0, 0);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, grid.pipeline);
        cameraController->push(commandBuffer, grid.layout);
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, gridBuffer, &offset);
        vkCmdDraw(commandBuffer, 6, 1, 0, 0);
        
        vkCmdEndRenderPass(commandBuffer);
    });
}

void CSOView::createBuffers() {
    std::vector<glm::vec3> simplex(30000);
    std::vector<uint16_t> indices(30000);
    csoBuffer = device->createCpuVisibleBuffer( simplex.data(), BYTE_SIZE(simplex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    csoIndexBuffer = device->createCpuVisibleBuffer(indices.data(), BYTE_SIZE(indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    glm::vec3 aOrigin{0};
    originBuffer = device->createDeviceLocalBuffer(&aOrigin, sizeof(glm::vec3), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

    std::vector<glm::vec3> gridLines{
        {-100, 0, 0}, {100, 0, 0},
        {0, -100, 0}, {0, 100, 0},
        {0, 0, -100}, {0, 0, 100},
    };
    gridBuffer = device->createDeviceLocalBuffer(gridLines.data(), BYTE_SIZE(gridLines), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}


void CSOView::createPipeline() {
//    @formatter:off
    auto builder = device->graphicsPipelineBuilder();
    cso.pipeline =
        builder
            .allowDerivatives()
            .shaderStage()
               .vertexShader(fileManager->load("flat.vert.spv"))
                .fragmentShader(fileManager->load("flat.frag.spv"))
            .vertexInputState()
                .addVertexBindingDescription(0, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX)
                .addVertexAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0)
            .inputAssemblyState()
                .triangles()
            .viewportState()
                .viewport()
                    .origin(0, 0)
                    .dimension({ width, height})
                    .minDepth(0)
                    .maxDepth(1)
                .scissor()
                    .offset(0, 0)
                    .extent({ width, height })
                .add()
            .rasterizationState()
                .cullNone()
                .frontFaceCounterClockwise()
                .polygonModeLine()
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
            .renderPass(renderPass)
            .subpass(0)
            .name("cso")
        .build(cso.layout);

    origin.pipeline =
        builder
            .basePipeline(cso.pipeline)
            .inputAssemblyState()
                .points()
            .name("origin")
        .build(origin.layout);

    grid.pipeline =
        builder
            .basePipeline(cso.pipeline)
            .inputAssemblyState()
                .lines()
            .name("grid")
        .build(grid.layout);


//    @formatter:on
}

void CSOView::update(float time) {
    checkAppInput();
    cameraController->update(time);
    render();
}

void CSOView::initCamera() {
    OrbitingCameraSettings settings{};
    settings.offsetDistance = 10.0f;
    settings.orbitMinZoom = 0.1f;
    settings.orbitMaxZoom = 10.f;
    settings.rotationSpeed = 0.1f;
    settings.zNear = 0.1f;
    settings.zFar = 20.0f;
    settings.fieldOfView = 45.0f;
    settings.modelHeight = 0;
    settings.aspectRatio = static_cast<float>(width)/static_cast<float>(height);
    cameraController = std::make_unique<OrbitingCameraController>(  *inputManager, settings);
}

void CSOView::checkAppInput() {
    cameraController->processInput();
}

void CSOView::update(std::vector<glm::vec3> &simplex, std::vector<uint16_t> &indices) {
    simplexSize = indices.size();
//    assert(simplexSize <= 4);
    auto vPtr = reinterpret_cast<glm::vec3*>(csoBuffer.map());
    for(int i = 0; i < simplex.size(); i++){
        vPtr[i] = simplex[i];
    }
    auto iPtr = reinterpret_cast<uint16_t*>(csoIndexBuffer.map());
    for(int i = 0; i < indices.size(); i++){
        iPtr[i] = indices[i];
    }
    csoBuffer.unmap();
    render();
}
