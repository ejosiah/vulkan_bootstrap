#pragma once

#include "common.h"
#include "VulkanDevice.h"

namespace vkn {

    struct Primitive {
        Primitive() = default;

        Primitive(uint32_t firstIndex, uint32_t vertexCount, uint32_t firstVertex, uint32_t indexCount, uint32_t vertexOffset)
                : firstIndex(firstIndex)
                , indexCount(indexCount)
                , firstVertex(firstVertex)
                , vertexCount(vertexCount)
                , vertexOffset(vertexOffset)
                {};

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
            if(indexCount > 0){
                vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
            }else{
                vkCmdDraw(commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
            }
        }
    };

}