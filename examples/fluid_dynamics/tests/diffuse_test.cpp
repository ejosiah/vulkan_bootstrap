#include "fluid_sim_fixture.hpp"

class DiffuseTest : public FluidSimFixture {
public:
    DiffuseTest()
            :FluidSimFixture(){}

protected:

    void SetUp() override{
        N(8);
        dt(0.125);
        FluidSimFixture::SetUp();
    }
};