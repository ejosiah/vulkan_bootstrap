#pragma once

#include "common.h"

struct Copyable{

    void copy(void* source, VkDeviceSize size) const {
        void* dest;
        vmaMapMemory(Allocator(), Allocation(), &dest);
        memcpy(dest, source, size);
        vmaUnmapMemory(Allocator(), Allocation());
    }

    virtual VmaAllocator Allocator() const = 0;

    virtual VmaAllocation Allocation() const = 0;
};