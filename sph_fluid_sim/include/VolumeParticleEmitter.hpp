#pragma once

#include "ComputePipelins.hpp"
#include "Texture.h"
#include "primitives.h"
#include "model.hpp"
#include "PointGenerator.hpp"

class VolumeParticleEmitter : public PointGenerator{
public:
    VolumeParticleEmitter() = default;

    explicit VolumeParticleEmitter(VulkanDevice* device, VulkanDescriptorPool* pool, VulkanDescriptorSetLayout* particleDescriptorSetLayout
                                   , Sdf& sdf, float spacing = 1.0, PointGeneratorType genType = BCC_LATTICE_POINT_GENERATOR);


    [[nodiscard]]
    std::string shaderName() const override{
        return "volume_emitter";
    }

    void createDescriptorSetLayout() override;

    void createDescriptorSet() override;

private:
    Texture* texture{nullptr};
};