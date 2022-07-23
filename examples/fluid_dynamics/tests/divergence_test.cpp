#include "fluid_sim_fixture.hpp"
#define LOC(i, j) ((i)*(N+2)+(j))

using namespace glm;

class DivergenceTest : public FluidSimFixture {
public:
    DivergenceTest()
            :FluidSimFixture(){}

    void SetUp() override {
        _constants.N = 128;
        FluidSimFixture::SetUp();
    }
};

TEST_F(DivergenceTest, divergenceOfVectorFieldShouldBeCorrect){

    auto f = [](float x, float y){
        return cos(x) * sin(y);
    };

    auto g = [](float x, float y){
        return sin(x) * sin(y);
    };

    const auto N = _constants.N;
    std::vector<vec2> field;
    field.reserve((N+2) * (N+2));

    for(int j = 0; j < N + 2; j++){
        for(int i = 0; i < N+2; i++){
            float x = 2.f * static_cast<float>(j)/static_cast<float>(N) - 1;
            float y = 2.f * static_cast<float>(i)/static_cast<float>(N) - 1;

            vec2 u{f(x, y), g(x, y)};
            field.push_back(u);
        }
    }

    setVectorField(field);

    context().device.computeCommandPool().oneTimeCommand([&](auto commandBuffer){
        fluidSim.calculateDivergence(commandBuffer);
    });



    auto u = reinterpret_cast<float*>(_u0.map());
    auto v = reinterpret_cast<float*>(_v0.map());
    auto div = reinterpret_cast<float*>(fluidSim._divBuffer.map());

    float delta = 1/float(N);
    for(int i = 1; i <= N; i++){
        for(int j = 1; j <= N; j++){

            float dudx = 0.5f * (u[LOC(i, j+1)] - u[LOC(i, j-1)]) / delta;
            float dudy = 0.5f * (v[LOC(i+1, j)] - v[LOC(i-1, j)]) / delta;
            float expected = dudx + dudy;
            ASSERT_NEAR(expected, div[LOC(i, j)], 0.001);
        }
    }

}