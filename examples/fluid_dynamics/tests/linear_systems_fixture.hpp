#pragma once

#include "random.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <array>
#include <fmt/format.h>

class LinearSystemsFixture : public ::testing::Test {
public:
    LinearSystemsFixture(){}

protected:

    void SetUp() override {

    }

    void TearDown() override {

    }

    static constexpr int N = 4;

    std::array<float, N * N> A{
        10., -1., 2., 0.,
        -1., 11., -1., 3,
        2., -1., 10., -1,
        0.0, 3., -1., 8.
    };
    std::array<float, N> b{6, 25, -11, 15};
    std::array<float, N> x{1, 2, -1, 1};
    int seed = 1 << 20;
};