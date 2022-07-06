#include "fluid_sim_fixture.hpp"

#define IX(i, j) (int(i)+(N+2)*int(j))

using namespace glm;

class GradientTest : public FluidSimFixture {
public:
    GradientTest()
            : FluidSimFixture() {}

    void SetUp() override {
        _constants.N = 128;
        FluidSimFixture::SetUp();
    }

};

TEST_F(GradientTest, gradientOfScalarFieldShoudlBeCorrect){
    FAIL() << "Not yet implemented!";
}