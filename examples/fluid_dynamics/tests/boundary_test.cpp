#include "fluid_sim_fixture.hpp"
#define IX(i, j) ((i)+(N+2)*(j))

using namespace glm;

class BoundaryTest : public FluidSimFixture {
public:
    BoundaryTest()
            : FluidSimFixture() {}

};

TEST_F(BoundaryTest, vectorFieldBoundaryCheck){
    NOT_YET_IMPLEMENTED;
}

TEST_F(BoundaryTest, scalaFieldBoundaryCheck){
    NOT_YET_IMPLEMENTED;
}