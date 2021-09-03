#include "VulkanFixture.hpp"
#include "PrefixSum.hpp"

class PrefixScanTest : public VulkanFixture{
protected:

    void postVulkanInit() override {
        _prefix_sum = PrefixSum{ &device };
        _prefix_sum.init();
    }

protected:
    PrefixSum _prefix_sum;
};


TEST_F(PrefixScanTest, ScanWithSingleWorkGroup){
    std::vector<int> data(8 << 10);
    auto rng = rngFunc<int>(0, 100, 1 << 20);
    std::generate(begin(data), end(data), [&]{ return rng(); });

    auto expected = data;
    std::exclusive_scan(begin(expected), end(expected), begin(expected), 0);

    _prefix_sum.scan(begin(data), end(data));

    for(int i = 0; i < data.size(); i++){
        ASSERT_EQ(expected[i], data[i]);
    }

}

TEST_F(PrefixScanTest, ScanDataItemsLessThanWorkGroupSize){
    std::vector<int> data(8 << 9);
    auto rng = rngFunc<int>(0, 100, 1 << 20);
    std::generate(begin(data), end(data), [&]{ return rng(); });

    auto expected = data;
    std::exclusive_scan(begin(expected), end(expected), begin(expected), 0);

    _prefix_sum.scan(begin(data), end(data));

    for(int i = 0; i < data.size(); i++){
        ASSERT_EQ(expected[i], data[i]);
    }
}

TEST_F(PrefixScanTest, ScanNonPowerOfDataItems){
    std::vector<int> data(55555);
    auto rng = rngFunc<int>(0, 100, 1 << 20);
    std::generate(begin(data), end(data), [&]{ return rng(); });

    auto expected = data;
    std::exclusive_scan(begin(expected), end(expected), begin(expected), 0);

    _prefix_sum.scan(begin(data), end(data));

    for(int i = 0; i < data.size(); i++){
        ASSERT_EQ(expected[i], data[i]);
    }
}

TEST_F(PrefixScanTest, ScanWithMutipleWorkGroups){
    std::vector<int> data((8 << 10) + 1);
    auto rng = rngFunc<int>(0, 100, 1 << 20);
    std::generate(begin(data), end(data), [&]{ return rng(); });

    auto expected = data;
    std::exclusive_scan(begin(expected), end(expected), begin(expected), 0);

    _prefix_sum.scan(begin(data), end(data));

    for(int i = 0; i < data.size(); i++){
        ASSERT_EQ(expected[i], data[i]);
    }
}
