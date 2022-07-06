//
// Created by Josiah on 18/06/2021.
//

#include <gtest/gtest.h>
#include "fluid_dynamics_cpu.h"
#include "fluid_sim_fixture.hpp"

int factorial(int n){
    if(n <= 1) return 1;
    return n * factorial(n - 1);
}



TEST(FactorialTest, HandleZeroInput){
    EXPECT_EQ(factorial(0), 1);
}

TEST(FactorialTest, HandlesPositiveInput) {
    EXPECT_EQ(factorial(1), 1);
    EXPECT_EQ(factorial(2), 2);
    EXPECT_EQ(factorial(3), 6);
    EXPECT_EQ(factorial(8), 40320);
}

TEST(FluidSim, expectedBehaviour){
    int N = 4;
    int size = (N + 2) * (N + 2);
    float dt = 0.0625;
    std::vector<float> _q0(size, 0);
    std::vector<float> _q(size, 0);

    std::vector<float> _u(size, 0);
    std::vector<float> _v(size, 0);

    Accessor2D q0{ _q0.data(), N};
    Accessor2D q{ _q.data(), N};
    Accessor2D u{ _u.data(), N};
    Accessor2D v{ _v.data(), N};

    q0[2][2] = 38;
    q0[2][3] = 55;
    q0[3][2] = 12;
    q0[3][3] = 20;

    u[3][3]  = -8;
    v[3][3] = -8;

    advect(N, 0, q.data(), q0.data(), u.data(), v.data(), dt);

    ASSERT_EQ(9.5, q[3][3]);

}