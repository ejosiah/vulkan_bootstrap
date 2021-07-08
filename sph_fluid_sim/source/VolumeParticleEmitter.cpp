#include "VolumeParticleEmitter.hpp"

VolumeParticleEmitter::VolumeParticleEmitter(Texture *texture, BoundingBox bounds, float spacing,
                                             PointGeneratorType genType)
                                             : texture{ texture}
                                             , constants{std::get<0>(bounds), spacing, std::get<1>(bounds), int(genType) }
{

}

std::vector<PipelineMetaData> VolumeParticleEmitter::pipelineMetaData() {
    return {
        {
            "volume_particle_emitter",
            "../../data/sph/volume_emitter.comp.spv"
        }
    };
}

void VolumeParticleEmitter::execute(VkCommandBuffer commandBuffer) {

}

void VolumeParticleEmitter::setPointGenerator(PointGeneratorType genType) {
    constants.genType = int(genType)
}

void VolumeParticleEmitter::setSpacing(float space) {
    constants.spacing = space;
}
