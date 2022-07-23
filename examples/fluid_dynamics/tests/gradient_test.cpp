#include "fluid_sim_fixture.hpp"

#define LOC(i, j) (int(i) * (N+2) + int(j))

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
    const auto N = _constants.N;
    auto f = [](float x, float y){
        return glm::sin(x) * glm::sin(y);
    };

    std::vector<float> field;
    field.reserve((N+2) * (N+2));

    for(int j = 0; j < N + 2; j++){
        for(int i = 0; i < N+2; i++){
            float x = 2.f * static_cast<float>(j)/static_cast<float>(N) - 1;
            float y = 2.f * static_cast<float>(i)/static_cast<float>(N) - 1;

            auto z = f(x, y);
            field.push_back(z);
        }
    }

    setScalaField(field);

    context().device.computeCommandPool().oneTimeCommand([&](auto commandBuffer){
        fluidSim.computePressureGradient(commandBuffer);
    });

    auto pressure = reinterpret_cast<float*>(fluidSim._pressure.map());
    auto u = reinterpret_cast<float*>(fluidSim._gu.map());
    auto v = reinterpret_cast<float*>(fluidSim._gv.map());

    float delta = 1/float(N);
    for(int i = 1; i <= N; i++){
        for(int j = 1; j <= N; j++){

            float dudx = 0.5f * (pressure[LOC(i, j+1)] - pressure[LOC(i, j-1)]) / delta;
            float dudy = 0.5f * (pressure[LOC(i+1, j)] - pressure[LOC(i-1, j)]) / delta;
            ASSERT_NEAR(u[LOC(i, j)], dudx, 0.001);
            ASSERT_NEAR(v[LOC(i, j)], dudy, 0.001);
        }
    }

}