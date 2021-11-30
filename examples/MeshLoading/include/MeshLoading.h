#include "VulkanBaseApp.h"
#include "Mesh.h"

class MeshLoading : public VulkanBaseApp{
public:
    MeshLoading();

protected:
    void initApp() override;

    void onSwapChainDispose() override;

    void onSwapChainRecreation() override;

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override;

    void update(float time) override;

    void checkAppInputs() override;

private:
    VulkanCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
};