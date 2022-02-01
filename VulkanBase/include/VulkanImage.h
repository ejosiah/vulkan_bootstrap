#pragma once

#include "common.h"
#include "VulkanCopyable.h"
#include "VulkanCommandBuffer.h"
#include "VulkanRAII.h"

static const VkImageSubresourceRange DEFAULT_SUB_RANGE{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

struct VulkanImage : public Copyable{
    
    DISABLE_COPY(VulkanImage)

    VulkanImage() = default;

    inline VulkanImage(VkDevice device, VmaAllocator allocator, VkImage image, VkFormat format, VmaAllocation allocation, VkImageLayout layout, VkExtent3D dimension)
    : device(device)
    , allocator(allocator)
    , image(image)
    , format(format)
    , allocation(allocation)
    , currentLayout(layout)
    , dimension(dimension)
    {}

    VulkanImage(VulkanImage&& source) noexcept {
        operator=(static_cast<VulkanImage&&>(source));
    }

    VulkanImage& operator=(VulkanImage&& source) noexcept {
        if(&source == this) return *this;

        this->~VulkanImage();

        device = source.device;
        allocator = source.allocator;
        image = source.image;
        allocation = source.allocation;
        currentLayout = source.currentLayout;
        dimension = source.dimension;
        format = source.format;

        source.allocator = VK_NULL_HANDLE;
        source.image = VK_NULL_HANDLE;
        source.allocation = VK_NULL_HANDLE;
        source.currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        return *this;
    }

    ~VulkanImage(){
        if(image){
            vmaDestroyImage(allocator, image, allocation);
        }
    }

    operator VkImage() const {
        return image;
    }

    [[nodiscard]]
    VmaAllocator Allocator() const override {
        return allocator;
    }

    [[nodiscard]]
    VmaAllocation Allocation() const override {
        return allocation;
    }

    [[nodiscard]]
    glm::uvec3 getDimensions(){
        return {dimension.width, dimension.height, dimension.depth};
    }

    // TODO change from pool to commandBufffer
    void transitionLayout(const VulkanCommandPool& pool, VkImageLayout newLayout, const VkImageSubresourceRange& subresourceRange = DEFAULT_SUB_RANGE) {
        pool.oneTimeCommand([&](VkCommandBuffer commandBuffer) {

            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = currentLayout;
            barrier.newLayout = newLayout;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = image;
            barrier.subresourceRange = subresourceRange;

            if(newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL){
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                if(hasStencil(format)){
                    barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
                }
            }

            VkPipelineStageFlags sourceStage;
            VkPipelineStageFlags destinationStage;

            if (currentLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

                sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            } else if (currentLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
                       newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            }else if(currentLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL){
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

                sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            }
            else {
                throw std::runtime_error{"unsupported layout transition!"};
            }

            vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
            currentLayout = newLayout;
        });
    }

    void transitionLayout(const VulkanCommandPool& pool, VkImageLayout newLayout, const VkImageSubresourceRange& subresourceRange,
                          VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask){

        pool.oneTimeCommand( [&](auto commandBuffer) {
            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.srcAccessMask = srcAccessMask;
            barrier.dstAccessMask = dstAccessMask;
            barrier.oldLayout = currentLayout;
            barrier.newLayout = newLayout;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = image;
            barrier.subresourceRange = subresourceRange;

            vkCmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, 0,
                                 0, nullptr, 0, nullptr, 1, &barrier);
            currentLayout = newLayout;
        });
    }

    [[nodiscard]] VulkanImageView createView(VkFormat format, VkImageViewType viewType, const VkImageSubresourceRange& subresourceRange, VkImageViewCreateFlags flags = 0) const {
        VkImageViewCreateInfo  createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.viewType = viewType;
        createInfo.flags = flags;
        createInfo.image = image;
        createInfo.format = format;
        createInfo.subresourceRange = subresourceRange;

        VkImageView view;
        ERR_GUARD_VULKAN(vkCreateImageView(device, &createInfo, nullptr, &view));

        return VulkanImageView{ device, view };
    }

    VkDevice device = VK_NULL_HANDLE;
    VmaAllocator allocator = VK_NULL_HANDLE;
    VkImage image = VK_NULL_HANDLE;
    VkFormat format = VK_FORMAT_UNDEFINED;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VkImageLayout currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkExtent3D dimension = { 0u, 0u, 0u };

    VkDeviceSize  size = 0;
};