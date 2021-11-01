#pragma once

#include <common.h>
#include <btBulletDynamicsCommon.h>
#include "VulkanDevice.h"
#include "Camera.h"
#include "Plugin.hpp"

struct Line{
    glm::vec4 _from{0};
    glm::vec4 _to{0};
    glm::vec3 color{0};
};

struct ContactPoint{
    glm::vec3 pointOnA;
    glm::vec3 pointOnB;
    glm::vec3 normal;
    glm::vec3 color;
    float distance;
    int lifeTime;
};

constexpr int NUM_LINES = 100000;

class DebugDrawer : public btIDebugDraw{
public:
    explicit DebugDrawer(const PluginData& pluginData, int debugMode);

    void init();

    void createLineBuffer();

    void createGraphicsPipeline();

    void drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color) override;

    void drawContactPoint(const btVector3 &PointOnB, const btVector3 &normalOnB, btScalar distance, int lifeTime,
                          const btVector3 &color) override;

    void reportErrorWarning(const char *warningString) override;

    void draw3dText(const btVector3 &location, const char *textString) override;

    void setDebugMode(int debugMode) override;

    void set(PluginData pluginData);

    [[nodiscard]]
    int getDebugMode() const override;

    void clearLines() override;

    void flushLines() override;

    void draw(VkCommandBuffer commandBuffer);

    VulkanBuffer fillStagingBuffer();

    void setCamera(Camera* camera);

    int _debugMode = btIDebugDraw::DebugDrawModes::DBG_NoDebug;

    std::vector<Line> _lines;
    std::vector<ContactPoint> _contactPoints;
    Camera* _camera{};

    struct {
        struct { ;
            VulkanPipeline pipeLine;
            VulkanPipelineLayout layout;
            VkPushConstantRange range{VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Camera) + sizeof(glm::vec3)};
        } lines;
    } _pipelines;

    VulkanBuffer _lineBuffer;
    PluginData _pluginData;
};