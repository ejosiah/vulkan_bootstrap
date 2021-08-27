#include "SortFixture.hpp"

class RadixSortFixture : public SortFixture{
protected:
   RadixSort _sort;

    void postVulkanInit() override {
        _sort = RadixSort(&device);
        _sort.init();
    }

    void sort(VulkanBuffer& buffer){
        execute([&](auto commandBuffer){
           _sort(commandBuffer, buffer);
        });

    }
};

TEST_F(RadixSortFixture, sortGivenData){
    auto items = randomInts(1 << 20);
    VulkanBuffer buffer = device.createCpuVisibleBuffer(items.data(), BYTE_SIZE(items), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    sort(buffer);
    ASSERT_TRUE(isSorted(buffer)) << "buffer should be sorted";
}

TEST_F(RadixSortFixture, sortIsStable){
    FAIL() << "Not yet implemented!";
}