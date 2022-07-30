#include "fluid_solver_2d.h"
#include "Vertex.h"

FluidSolver2D::FluidSolver2D(VulkanDevice* device, FileManager *fileManager)
: device(device)
, fileManager(fileManager)
{

}

void FluidSolver2D::init() {
    initFullScreenQuad();
    createDescriptorPool();
    createDescriptorSetLayouts();
    updateDescriptorSets();
    createPipelines();
}

void FluidSolver2D::initFullScreenQuad() {
    auto quad = ClipSpace::Quad::positions;
    screenQuad.vertices = device->createDeviceLocalBuffer(quad.data(), BYTE_SIZE(quad), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

void FluidSolver2D::createDescriptorPool() {
    constexpr uint32_t maxSets = 100;
    std::array<VkDescriptorPoolSize, 1> poolSizes{
            {
                    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100 * maxSets }
            }
    };
    descriptorPool = device->createDescriptorPool(maxSets, poolSizes, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);
}
