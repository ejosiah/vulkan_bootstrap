#include "VulkanBaseApp.h"

struct VertexInput{
    glm::vec2 position;
    glm::vec2 uv;
    glm::vec4 color;
};

class ComputeDemo final : public VulkanBaseApp{
public:
    explicit ComputeDemo(const Settings& settings);

protected:
    void initApp() final;

    VkCommandBuffer& dispatchCompute();

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) final;

    void update(float time) final;

    void checkAppInputs() final;

    void loadTexture();

    void createVertexBuffer();

    void createDescriptorSetLayout();

    void createDescriptorPool();

    void createDescriptorSet();

    void createGraphicsPipeline();

    void createComputeDescriptorSetLayout();

    void createComputePipeline();

    void createComputeImage();

private:
    VulkanCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkCommandBuffer> cpuCmdBuffers;
    VulkanPipelineLayout pipelineLayout;
    VulkanPipeline pipeline;
    VulkanBuffer vertexBuffer;
    VulkanBuffer vertexColorBuffer;
    VulkanDescriptorPool descriptorPool;
    VulkanDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSet descriptorSet;
    Texture texture;
    struct {
        VulkanDescriptorSetLayout descriptorSetLayout;
        VulkanPipelineLayout pipelineLayout;
        VkDescriptorSet descriptorSet;
        VulkanPipeline pipeline;
        Texture texture;
    } compute;
};