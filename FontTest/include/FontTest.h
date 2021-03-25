#include "VulkanBaseApp.h"
#include "Font.h"

class FontTest : public VulkanBaseApp {
public:
    FontTest();

protected:
    void initApp() override;

    void update(float time) override;

    void onSwapChainDispose() override;

    void onSwapChainRecreation() override;

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override;

    void cleanup() override;

private:
    std::vector<VkCommandBuffer> commandBuffers;
    VulkanCommandPool commandPool;
    Font* font = nullptr;
};