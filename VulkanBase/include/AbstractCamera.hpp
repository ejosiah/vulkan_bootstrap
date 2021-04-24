#pragma once

#include <glm/glm.hpp>

struct Camera{
    glm::mat4 model = glm::mat4(1);
    glm::mat4 view = glm::mat4(1);
    glm::mat4 proj = glm::mat4(1);

    static constexpr VkPushConstantRange pushConstant() {
        return {VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Camera)};
    }

};

class AbstractCamera{
public:
    virtual ~AbstractCamera() = default;

    virtual void update(float time) = 0;

    virtual void processInput() = 0;

    virtual void lookAt(const glm::vec3 &eye, const glm::vec3 &target, const glm::vec3 &up) = 0;

    virtual void perspective(float fovx, float aspect, float znear, float zfar) = 0;

    virtual void perspective(float aspect) = 0;

    virtual void rotateSmoothly(float headingDegrees, float pitchDegrees, float rollDegrees) = 0;

    virtual void rotate(float headingDegrees, float pitchDegrees, float rollDegrees) = 0;

    virtual void move(float dx, float dy, float dz) = 0;

    virtual void move(const glm::vec3 &direction, const glm::vec3 &amount) = 0;

    virtual void position(const glm::vec3& pos) = 0;

    [[nodiscard]]
    virtual const glm::vec3& position() const = 0;

    [[nodiscard]]
    virtual const glm::vec3& velocity() const = 0;

    [[nodiscard]]
    virtual const glm::vec3& acceleration() const = 0;

    virtual void updatePosition(const glm::vec3 &direction, float elapsedTimeSec) = 0;

    virtual void undoRoll() = 0;

    virtual void zoom(float zoom, float minZoom, float maxZoom) = 0;

    virtual void onResize(int width, int height) = 0;

    virtual void setModel(const glm::mat4& model) = 0;

    virtual void push(VkCommandBuffer commandBuffer, VkPipelineLayout layout, VkShaderStageFlags stageFlags = VK_SHADER_STAGE_VERTEX_BIT) const = 0;

    virtual void push(VkCommandBuffer commandBuffer, VkPipelineLayout layout, const glm::mat4& model, VkShaderStageFlags stageFlags = VK_SHADER_STAGE_VERTEX_BIT) = 0;

    [[nodiscard]]
    virtual const glm::quat& getOrientation() const = 0;

    [[nodiscard]]
    virtual const Camera& cam() const = 0;
};