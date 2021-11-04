#include <DebugDrawer.hpp>
#include "VulkanShaderModule.h"
#include "GraphicsPipelineBuilder.hpp"

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
    _pipelines.lines.pipeLine =
        GraphicsPipelineBuilder(_pluginData.device)
            .shaderStage()
                .addVertexShader("../../data/shaders/bullet/debug.vert.spv")
                .addFragmentShader("../../data/shaders/bullet/debug.frag.spv")
            .vertexInputState()
                .addVertexBindingDescription(0, sizeof(glm::vec4), VK_VERTEX_INPUT_RATE_VERTEX)
                .addVertexAttributeDescription(0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 0)
            .inputAssemblyState()
                .lines()
            .viewportState()
                .viewport()
                    .origin(0, 0)
                    .dimension(_pluginData.swapChain->extent)
                    .minDepth(0)
                    .maxDepth(1)
                .scissor()
                    .offset(0, 0)
                    .extent(_pluginData.swapChain->extent)
                .add()
            .rasterizationState()
                .cullBackFace()
                .frontFaceClockwise()
                .polygonModeLine()
                .lineWidth(1.0)
            .depthStencilState()
                .enableDepthTest()
                .enableDepthWrite()
                .compareOpLess()
                .minDepthBounds(0.f)
                .maxDepthBounds(1.f)
             .colorBlendState()
                .attachment()
                    .disableBlend()
                    .colorBlendOp().add()
                    .srcAlphaBlendFactor().one()
                    .dstAlphaBlendFactor().one()
                    .srcColorBlendFactor().one()
                    .dstColorBlendFactor().oneMinusSrcAlpha()
                .add()
            .layout()
                .addPushConstantRange(_pipelines.lines.range)
            .renderPass(*_pluginData.renderPass)
            .subpass(0)
        .build(_pipelines.lines.layout);
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
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelines.lines.pipeLine);
        uint32_t numLines = _lines.size();

        VkDeviceSize offsets = 0u;
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, _lineBuffer, &offsets);
        for (int i = 0; i < numLines; i++) {
            auto &line = _lines[i];

            vkCmdPushConstants(commandBuffer, _pipelines.lines.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Camera), _camera);
            vkCmdPushConstants(commandBuffer, _pipelines.lines.layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(Camera),
                               sizeof(glm::vec3), &line.color);
            vkCmdDraw(commandBuffer, 2, 1, i * 2, 0);
        }
    }
}
