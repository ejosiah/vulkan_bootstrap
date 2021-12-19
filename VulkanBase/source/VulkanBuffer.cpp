#include "VulkanBuffer.h"

std::map<VkBuffer, std::atomic_uint32_t> VulkanBuffer::refCounts;