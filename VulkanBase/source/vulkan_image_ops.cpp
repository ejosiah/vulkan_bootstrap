#include "vulkan_image_ops.h"

#include <utility>

class VulkanImageOps::Impl{
public:
    Impl(VulkanImageOps* ops = nullptr, VulkanDevice* device = nullptr)
    : _ops{ ops }
    , _device{ device }
    {}

    VulkanImageOps &srcTexture(Texture &texture){
        return *_ops;
    }

    VulkanImageOps &dstTexture(Texture &texture){
        return *_ops;
    }

    VulkanImageOps& srcBuffer(VulkanBuffer buffer){
        _srcBuffer = std::move(buffer);
        return *_ops;
    }

    VulkanImageOps& imageSubresourceRange(VkImageAspectFlags aspectMask,
                                          uint32_t baseMipLevel,
                                          uint32_t levelCount,
                                          uint32_t baseArrayLayer,
                                          uint32_t layerCount){
        _subresourceRange.aspectMask = aspectMask;
        _subresourceRange.baseMipLevel = baseMipLevel;
        _subresourceRange.levelCount = levelCount;
        _subresourceRange.baseArrayLayer = baseArrayLayer;
        _subresourceRange.layerCount = layerCount;

        return *_ops;
    }

    VulkanImageOps& sourceSrcPipelineStage(VkPipelineStageFlags flags){
        _sourceSrcPipelineStage = flags;
        return *_ops;
    }

    VulkanImageOps& srcPipelineStage(VkPipelineStageFlags flags){
        _srcPipelineStage = flags;
        return *_ops;
    }

    VulkanImageOps& dstPipelineStage(VkPipelineStageFlags flags){
        _dstPipelineStage = flags;
        return *_ops;
    }

    VulkanImageOps& sourceSrcAccessMask(VkAccessFlags flags){
        _sourceSrcAccessMask = flags;
        return *_ops;
    }
    VulkanImageOps& srcAccessMask(VkAccessFlags flags){
        _srcAccessMask = flags;
        return *_ops;
    }

    VulkanImageOps& dstAccessMask(VkAccessFlags flags){
        _dstAccessMask = flags;
        return *_ops;
    }

    void copy(VkCommandBuffer commandBuffer){
        assert(_srcTexture);
        assert(_dstTexture);
        assert(_sourceSrcPipelineStage != VK_PIPELINE_STAGE_NONE);
        assert(_srcPipelineStage != VK_PIPELINE_STAGE_NONE);
        assert(_srcAccessMask != VK_ACCESS_NONE);
        assert(_sourceSrcAccessMask != VK_ACCESS_NONE);

        _dstPipelineStage = (_dstPipelineStage == VK_PIPELINE_STAGE_NONE) ? _srcPipelineStage : _dstPipelineStage;
        _dstAccessMask = (_dstAccessMask == VK_ACCESS_NONE) ? _srcAccessMask : _dstAccessMask;

        auto dstOldLayout = _dstTexture->image.currentLayout;
        if(dstOldLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL){
            _dstTexture->image.transitionLayout(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, _subresourceRange
                    , _srcAccessMask, VK_ACCESS_TRANSFER_WRITE_BIT
                    , _srcPipelineStage, VK_PIPELINE_STAGE_TRANSFER_BIT);
        }

        auto srcOldLayout = _srcTexture->image.currentLayout;
        if(srcOldLayout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL){
            _srcTexture->image.transitionLayout(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, _subresourceRange
                    , _sourceSrcAccessMask, VK_ACCESS_TRANSFER_READ_BIT
                    , _sourceSrcPipelineStage, VK_PIPELINE_STAGE_TRANSFER_BIT);
        }

        VkImageSubresourceLayers imageSubresource{_subresourceRange.aspectMask, _subresourceRange.baseMipLevel,
                                                  _subresourceRange.baseArrayLayer, _subresourceRange.layerCount};

        VkImageCopy region{};
        region.srcSubresource = imageSubresource;
        region.srcOffset = {0, 0, 0};
        region.dstSubresource = imageSubresource;
        region.dstOffset = {0, 0, 0};
        region.extent = {_srcTexture->width, _srcTexture->height, 1};

        vkCmdCopyImage(commandBuffer, _srcTexture->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, _dstTexture->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        if(dstOldLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL){
            _dstTexture->image.transitionLayout(commandBuffer, dstOldLayout, _subresourceRange
                    ,  VK_ACCESS_TRANSFER_WRITE_BIT, _dstAccessMask
                    ,  VK_PIPELINE_STAGE_TRANSFER_BIT, _dstPipelineStage);
        }

        if(srcOldLayout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL){
            _srcTexture->image.transitionLayout(commandBuffer, srcOldLayout, _subresourceRange
                    ,  VK_ACCESS_TRANSFER_WRITE_BIT, _sourceSrcAccessMask
                    ,  VK_PIPELINE_STAGE_TRANSFER_BIT, _sourceSrcPipelineStage);
        }

    }

    void transfer(VkCommandBuffer commandBuffer){

    }

private:
    VulkanDevice *_device{nullptr};
    VulkanImageOps* _ops{nullptr};
    Texture *_srcTexture{nullptr};
    Texture *_dstTexture{nullptr};
    VulkanBuffer _srcBuffer{};
    VkPipelineStageFlags _srcPipelineStage{VK_PIPELINE_STAGE_NONE};
    VkPipelineStageFlags _dstPipelineStage{VK_PIPELINE_STAGE_NONE};
    VkAccessFlags _sourceSrcAccessMask{VK_ACCESS_NONE};
    VkAccessFlags _srcAccessMask{VK_ACCESS_NONE};
    VkAccessFlags _dstAccessMask{VK_ACCESS_NONE};
    VkImageSubresourceRange _subresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    VkPipelineStageFlags _sourceSrcPipelineStage{VK_PIPELINE_STAGE_NONE};
};

VulkanImageOps::VulkanImageOps(VulkanDevice *device)
: pimpl{new Impl{ this, device }}
{

}

VulkanImageOps &VulkanImageOps::srcTexture(Texture &texture) {
    return pimpl->srcTexture(texture);
}

VulkanImageOps &VulkanImageOps::dstTexture(Texture &texture) {
    return pimpl->dstTexture(texture);
}

VulkanImageOps &VulkanImageOps::srcBuffer(VulkanBuffer buffer) {
    return pimpl->srcBuffer(std::move(buffer));
}

VulkanImageOps &
VulkanImageOps::imageSubresourceRange(VkImageAspectFlags aspectMask, uint32_t baseMipLevel, uint32_t levelCount,
                                      uint32_t baseArrayLayer, uint32_t layerCount) {
    return pimpl->imageSubresourceRange(aspectMask, baseMipLevel, levelCount, baseMipLevel, layerCount);
}

VulkanImageOps &VulkanImageOps::sourceSrcPipelineStage(VkPipelineStageFlags flags) {
    return pimpl->sourceSrcPipelineStage(flags);
}

VulkanImageOps &VulkanImageOps::srcPipelineStage(VkPipelineStageFlags flags) {
    return pimpl->srcPipelineStage(flags);
}

VulkanImageOps &VulkanImageOps::dstPipelineStage(VkPipelineStageFlags flags) {
    return pimpl->dstPipelineStage(flags);
}

VulkanImageOps &VulkanImageOps::sourceSrcAccessMask(VkAccessFlags flags) {
    return pimpl->sourceSrcAccessMask(flags);
}

VulkanImageOps &VulkanImageOps::srcAccessMask(VkAccessFlags flags) {
    return pimpl->srcAccessMask(flags);
}

VulkanImageOps &VulkanImageOps::dstAccessMask(VkAccessFlags flags) {
    return pimpl->dstAccessMask(flags);
}

void VulkanImageOps::copy(VkCommandBuffer commandBuffer) {
    pimpl->copy(commandBuffer);
}

void VulkanImageOps::transfer(VkCommandBuffer commandBuffer) {
    pimpl->transfer(commandBuffer);
}


VulkanImageOps VulkanDevice::imageOps() {
    return VulkanImageOps{this};
}