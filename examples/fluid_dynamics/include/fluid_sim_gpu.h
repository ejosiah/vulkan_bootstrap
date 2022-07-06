#pragma once

#include "VulkanDevice.h"
#include <array>
#include "ComputePipelins.hpp"
#include "filemanager.hpp"
#include "Texture.h"

class FluidSim : public ComputePipelines{
public:
    FluidSim() = default;

    FluidSim(VulkanDevice* device, FileManager fileMgr,  VulkanBuffer u0, VulkanBuffer v0,
             VulkanBuffer u, VulkanBuffer v, VulkanBuffer q0, VulkanBuffer q1,
             int N, float dt, float dissipation);


    void init();

    void set(Texture& color);

    void initTextureSampler();

    void createDescriptorPool();

    std::vector<PipelineMetaData> pipelineMetaData() override;

    void createDescriptorSets();

    void updateDescriptorSets();

    void run(float speed);

    void advect(VkCommandBuffer commandBuffer);

    void dissipation(float value);

    void setQuantity(VulkanBuffer q0, VulkanBuffer q);

    void resize(VulkanBuffer u0, VulkanBuffer v0,
                VulkanBuffer u, VulkanBuffer v,
                VulkanBuffer q0, VulkanBuffer q1,
                int N);

    std::string resource(const std::string& path);

private:
    VulkanDescriptorPool _descriptorPool;
    VulkanDescriptorSetLayout _velocitySetLayout;
    VulkanDescriptorSetLayout _quantitySetLayout;
    std::array<VkDescriptorSet, 2> _velocityDescriptorSet{};
    std::array<VkDescriptorSet, 2> _quantityDescriptorSet{};
    VulkanBuffer _u0, _v0, _u, _v;
    VulkanBuffer _q0, _q;
    VulkanSampler textureSampler;

    int q_in = 0;
    int q_out = 1;
    int v_in = 0;
    int v_out = 1;
    int color_in = 0;
    int color_out = 1;

    struct {
        int N;
        float dt{1};
        float dissipation{1};
    } _constants{};
    std::array<int, 2> sc{};
    FileManager _fileManager;
    std::array<Texture, 2> scratchColors;
    Texture* _outputColor{nullptr};
};