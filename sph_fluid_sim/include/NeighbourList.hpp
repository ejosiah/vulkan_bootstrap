#include "VulkanDevice.h"
#include "ComputePipelins.hpp"
#include "PointHashGrid.hpp"
#include "model.hpp"

class NeighbourList : public ComputePipelines{
public:
    NeighbourList() = default;

    NeighbourList(VulkanDevice* device, VulkanDescriptorPool* descriptorPool,PointHashGrid& grid, uint32_t numParticles, glm::vec3 resolution, float gridSpacing);

    void init();


    struct {
        glm::vec3 resolution{1};
        float gridSpacing{1};
        uint32_t pass{0};
        uint32_t numParticles{0};
    } constants{};

    struct {
        VkDescriptorSet descriptorSet{};
        VkDescriptorSet sumScanDescriptorSet{};
        VulkanDescriptorSetLayout setLayout{};
        VulkanBuffer sumsBuffer{};
        struct {
            int itemsPerWorkGroup = 8 << 10;
            int N = 0;
        } constants{};
    } prefixScan{};
};