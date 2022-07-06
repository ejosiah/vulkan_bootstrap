#include "fluid_sim_fixture.hpp"

class AdvectTest : public FluidSimFixture {
public:
    AdvectTest()
    :FluidSimFixture(){}
};

TEST_F(AdvectTest, advectionShouldProduceTheCorrectQuantity){

    q0[1][1] = 5;
    q0[1][2] = 7;

    q0[2][1] = 12;
    q0[2][2] = 10;

    u0[2][2]  = 1;
    v0[2][2] = 1;


    advect();

    for(int i = 0; i < _constants.N; i++){
        for(int j = 0; j < _constants.N; j++){
            fmt::print("{} ", q0[i][j]);
        }
        fmt::print("\n");
    }

    ASSERT_EQ(9.5, q[2][2]);
}

TEST_F(AdvectTest, advectionShouldProduceTheCorrectQuantityWithUnevenVector){
    q0[1][1] = 5;
    q0[1][2] = 7;
    q0[2][1] = 12;
    q0[2][2] = 10;

    u0[2][2]  = 1;
    v0[2][2] = 0.75;

    advect();

    ASSERT_EQ(9.75, q[2][2]);
}

TEST_F(AdvectTest, advectionShouldProduceCorrectQuantityAndLowerBoundary){
    q0[0][0] = 6;
    q0[0][1] = 12;
    q0[1][0] = 2;
    q0[1][1] = 2;

    u0[1][0]  = 8;
    v0[1][0] = 8;

    advect();

    ASSERT_EQ(1.5, q[1][0]);
}

TEST_F(AdvectTest, advectionShouldProduceCorrectQuantityAndUpperBoundary){
    q0[2][2] = 38;
    q0[2][3] = 55;
    q0[3][2] = 12;
    q0[3][3] = 20;

    u0[3][3]  = -8;
    v0[3][3] = -8;

    advect();

    ASSERT_EQ(5, q[3][3]);
}

TEST_F(AdvectTest, advectionShouldDissipationOnAvectedQuantity){

    q0[1][1] = 5;
    q0[1][2] = 7;

    q0[2][1] = 12;
    q0[2][2] = 10;

    u0[2][2]  = 1;
    v0[2][2] = 1;

    dissipation(0.5);

    advect();


    ASSERT_EQ(4.75, q[2][2]);
}