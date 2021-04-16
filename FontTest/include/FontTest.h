#include "VulkanBaseApp.h"
#include "VulkanSink.h"

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
    std::string msg;
    std::shared_ptr<VulkanSink<spdlog::details::null_mutex, 20>> cappedSink;
};