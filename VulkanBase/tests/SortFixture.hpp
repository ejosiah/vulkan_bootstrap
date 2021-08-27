#pragma

#include "Sort.hpp"
#include "VulkanFixture.hpp"

class SortFixture : public VulkanFixture{
protected:
    [[nodiscard]]
    std::vector<uint32_t> randomInts(int numElements, uint32_t limit = std::numeric_limits<uint32_t>::max()) const {
        auto rng = rngFunc<uint32_t>(0, limit - 1, 1 << 20);
        std::vector<uint32_t> hostBuffer(numElements);
        std::generate(begin(hostBuffer), end(hostBuffer), rng);

        return hostBuffer;
    }

    bool isSorted(VulkanBuffer& buffer) const {
        auto begin = reinterpret_cast<uint32_t*>(buffer.map());
        auto end = begin + buffer.size/sizeof(uint32_t);
        auto sorted = std::is_sorted(begin, end);
        buffer.unmap();
        return sorted;
    }

    bool isStable(VulkanBuffer& buffer, VulkanBuffer indexBuffer) const {
        bool result = true;
        int numElements = buffer.size/sizeof(uint32_t);

        buffer.map<uint32_t>([&](auto valuePtr){
            indexBuffer.map<uint32_t>([&](auto indexPtr){
                for(int i = 0; i < numElements - 1; i++){
                    auto a = *(valuePtr + i);
                    auto b = *(valuePtr + i + 1);
                    if(a == b){
                        auto aIndex = *(indexPtr + i);
                        auto bIndex = *(indexPtr + i + 1);
                        result = aIndex < bIndex;
                        if(!result) break;
                    }
                }
            });
        });
        return result;
    }
};