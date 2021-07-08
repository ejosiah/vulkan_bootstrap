#pragma once

#include "ComputePipelins.hpp"
#include "Texture.h"
#include "primitives.h"

enum PointGeneratorType{
 GRID_POINT_GENERATOR = 0,
 BCC_LATTICE_POINT_GENERATOR = 1,
 FCC_LATTICE_POINT_GENERATOR  = 2
};

using BoundingBox = std::tuple<glm::vec3, glm::vec3>;

class VolumeParticleEmitter : public ComputePipelines{
public:
    VolumeParticleEmitter() = default;

    explicit VolumeParticleEmitter(Texture* texture, BoundingBox bounds, float spacing, PointGeneratorType genType);

    std::vector<PipelineMetaData> pipelineMetaData() final;

    void execute(VkCommandBuffer commandBuffer);

    void setPointGenerator(PointGeneratorType genType);

    void setSpacing(float space);


private:
    Texture* texture{nullptr};

    struct{
        glm::vec3 boundingBoxLowerCorner;
        float spacing;
        glm::vec3 boundingBoxUpperCorner;
        int genType;
    } constants;
};