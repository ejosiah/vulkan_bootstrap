#pragma once

#include "builder_forwards.hpp"
#include "VulkanDevice.h"

class Builder{
public:
    explicit Builder(VulkanDevice* device, Builder* parent = nullptr)
    : _parent{ parent }
    , _device{ device }
    {}

    Builder() = default;

    [[nodiscard]]
    virtual Builder* parent() {
        return _parent;
    }


    [[nodiscard]]
    VulkanDevice& device() const {
        return *_device;
    }

protected:
    VulkanDevice* _device = nullptr;
    Builder* _parent{nullptr };
};