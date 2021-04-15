#pragma once

#include "common.h"
#include "VulkanDevice.h"

namespace vkn {

    struct Primitive {
        Primitive() = default;

        static inline Primitive Vertices(uint32_t firstVertex, uint32_t vertexCount) {
            return {0, vertexCount, firstVertex, 0, 0};
        }

        static inline Primitive indexed(uint32_t indexCount, uint32_t firstIndex, uint32_t vertexOffset) {
            return {firstIndex, 0, 0, indexCount, vertexOffset};
        }

        struct {
            glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
            glm::vec3 max = glm::vec3(std::numeric_limits<float>::min());
        } bounds;

        void setBounds(glm::vec3 min, glm::vec3 max) {
            bounds.min = min;
            bounds.max = max;
        }

        uint32_t firstIndex = 0;
        uint32_t indexCount = 0;
        uint32_t firstVertex = 0;
        uint32_t vertexCount = 0;
        uint32_t vertexOffset = 0;

        inline void draw(VkCommandBuffer commandBuffer, uint32_t firstInstance = 0, uint32_t instanceCount = 1) const {
            vkCmdDraw(commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
        }

        inline void drawIndexed(VkCommandBuffer commandBuffer, uint32_t firstInstance = 0, uint32_t instanceCount = 1) const {
            vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
        }

    private:
        Primitive(uint32_t firstIndex, uint32_t vertexCount, uint32_t firstVertex, uint32_t indexCount, uint32_t vertexOffset)
                : firstIndex(firstIndex)
                , indexCount(indexCount)
                , firstVertex(firstVertex)
                , vertexCount(vertexCount)
                , vertexOffset(vertexOffset)
        {};
    };

}