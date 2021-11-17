#pragma once
#include "common.h"

struct SubpassDescription{
    VkSubpassDescriptionFlags flags{0};
    VkPipelineBindPoint pipelineBindPoint{VK_PIPELINE_BIND_POINT_GRAPHICS};
    std::vector<VkAttachmentReference> inputAttachments;
    std::vector<VkAttachmentReference> colorAttachments;
    std::vector<VkAttachmentReference> resolveAttachments;
    VkAttachmentReference depthStencilAttachments{};
    std::vector<uint32_t> preserveAttachments;

    inline operator VkSubpassDescription() const {
        VkSubpassDescription description{};
        description.flags = flags;
        description.pipelineBindPoint = pipelineBindPoint;
        description.inputAttachmentCount = COUNT(inputAttachments);
        description.pInputAttachments = inputAttachments.data();
        description.colorAttachmentCount = COUNT(colorAttachments);
        description.pColorAttachments = colorAttachments.data();
        description.pResolveAttachments = resolveAttachments.empty() ? nullptr : resolveAttachments.data();
        description.pDepthStencilAttachment = depthStencilAttachments.layout == VK_IMAGE_LAYOUT_UNDEFINED ? nullptr : &depthStencilAttachments;
        description.preserveAttachmentCount = COUNT(preserveAttachments);
        description.pPreserveAttachments = preserveAttachments.data();

        return description;
    }
};

