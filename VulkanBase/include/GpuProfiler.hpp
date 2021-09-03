#pragma once

#include "VulkanDevice.h"

struct GpuProfiler{
    GpuProfiler() = default;

    GpuProfiler(const VulkanDevice& device);

    const VulkanDevice& device;
};