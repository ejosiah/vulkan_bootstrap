#include <DebugDrawer.hpp>
#include <LinearMath/btVector3.h>
#include <spdlog/spdlog.h>
#include "vulkan_util.h"
#include "VulkanShaderModule.h"

DebugDrawer::DebugDrawer(const PluginData &pluginData, int aDebugMode)
: _pluginData{ pluginData }
, _debugMode{ aDebugMode }
{
    init();
}


void DebugDrawer::init() {
    createLineBuffer();
    createGraphicsPipeline();
}

void DebugDrawer::drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color) {
    Line line{ glm::vec4{from.getX(), from.getY(), from.getZ(), 1.f},
               glm::vec4{to.getX(), to.getY(), to.getZ(), 1.f},
               glm::vec3{color.getX(), color.getY(), color.getZ()}};
    _lines.push_back(line);
}

void
DebugDrawer::drawContactPoint(const btVector3 &PointOnB, const btVector3 &normalOnB, btScalar distance, int lifeTime,
                              const btVector3 &color) {

}

void DebugDrawer::reportErrorWarning(const char *warningString) {
    spdlog::warn(warningString);
}

void DebugDrawer::draw3dText(const btVector3 &location, const char *textString) {

}

void DebugDrawer::setDebugMode(int aDebugMode) {
    _debugMode = aDebugMode;
}

int DebugDrawer::getDebugMode() const {
    return _debugMode;
}

void DebugDrawer::clearLines() {
    _lines.clear();
}

void DebugDrawer::flushLines() {
    if(!_lines.empty()) {
        auto stagingBuffer = fillStagingBuffer();
        _pluginData.device->copy(stagingBuffer, _lineBuffer, stagingBuffer.size);
    }
}

void DebugDrawer::setCamera(Camera* camera) {
    _camera = camera;
}

VulkanBuffer DebugDrawer::fillStagingBuffer() {
    static std::vector<glm::vec4> data;
    data.clear();
    for(auto& line : _lines){
        data.push_back(line._from);
        data.push_back(line._to);
    }
    VkDeviceSize size = sizeof(glm::vec4) * data.size();
    VulkanBuffer stagingBuffer = _pluginData.device->createStagingBuffer(size);
    stagingBuffer.copy(data.data(), size);
    return stagingBuffer;
}

void DebugDrawer::set(PluginData pluginData) {
    _pluginData = pluginData;
}

void DebugDrawer::createGraphicsPipeline() {
    auto& device = *_pluginData.device;
    auto vertModule = VulkanShaderModule{"../../data/shaders/bullet/debug.vert.spv", device };
    auto fragModule = VulkanShaderModule{"../../data/shaders/bullet/debug.frag.spv", device };

    auto shaderStages = initializers::vertexShaderStages({
         { vertModule, VK_SHADER_STAGE_VERTEX_BIT },
         {fragModule, VK_SHADER_STAGE_FRAGMENT_BIT}
    });
    std::vector<VkVertexInputBindingDescription> bindings;
    bindings.push_back({0, sizeof(glm::vec4), VK_VERTEX_INPUT_RATE_VERTEX});

    std::vector<VkVertexInputAttributeDescription> attributes;
    attributes.push_back({0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 0});

    auto vertexInputState = initializers::vertexInputState(bindings, attributes);
    auto inputAssemblyState = initializers::inputAssemblyState(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);

    auto viewport = initializers::viewport(_pluginData.swapChain->extent);
    auto scissor = initializers::scissor(_pluginData.swapChain->extent);
    auto viewportState = initializers::viewportState(viewport, scissor);

    auto rasterState = initializers::rasterizationState();
    rasterState.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterState.polygonMode = VK_POLYGON_MODE_LINE;
    rasterState.lineWidth = 3.0;

    auto multisampleState = initializers::multisampleState();

    auto depthStencilState = initializers::depthStencilState();
    depthStencilState.depthTestEnable = VK_TRUE;
    depthStencilState.depthWriteEnable = VK_TRUE;
    depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencilState.minDepthBounds = 0.0;
    depthStencilState.maxDepthBounds = 1.0;

    auto colorBlendAttachment = initializers::colorBlendStateAttachmentStates();
    colorBlendAttachment[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment[0].colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment[0].blendEnable = VK_TRUE;
    auto colorBlendState = initializers::colorBlendState(colorBlendAttachment);

    _pipeline.layout = device.createPipelineLayout({}, {_pipeline.range });

    auto createInfo = initializers::graphicsPipelineCreateInfo();
    createInfo.flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
    createInfo.stageCount = COUNT(shaderStages);
    createInfo.pStages = shaderStages.data();
    createInfo.pVertexInputState = &vertexInputState;
    createInfo.pInputAssemblyState = &inputAssemblyState;
    createInfo.pViewportState = &viewportState;
    createInfo.pRasterizationState = &rasterState;
    createInfo.pMultisampleState = &multisampleState;
    createInfo.pDepthStencilState = &depthStencilState;
    createInfo.pColorBlendState = &colorBlendState;
    createInfo.layout = _pipeline.layout;
    createInfo.renderPass = *_pluginData.renderPass;
    createInfo.subpass = 0;

    _pipeline.pipeLine = device.createGraphicsPipeline(createInfo);
}

void DebugDrawer::createLineBuffer() {
    _lineBuffer = _pluginData.device->createBuffer(
                        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                        VMA_MEMORY_USAGE_GPU_ONLY,
                        NUM_LINES * sizeof(glm::vec4) * 2,
                        "line_buffer");
}

void DebugDrawer::draw(VkCommandBuffer commandBuffer) {
    if(!_lines.empty()) {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline.pipeLine);
        uint32_t numLines = _lines.size();

        VkDeviceSize offsets = 0u;
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, _lineBuffer, &offsets);
        for (int i = 0; i < numLines; i++) {
            auto &line = _lines[i];

            vkCmdPushConstants(commandBuffer, _pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Camera), _camera);
            vkCmdPushConstants(commandBuffer, _pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(Camera),
                               sizeof(glm::vec3), &line.color);
            vkCmdDraw(commandBuffer, 2, 1, i * 2, 0);
        }
    }
}
