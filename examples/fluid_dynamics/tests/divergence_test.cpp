#include "fluid_sim_fixture.hpp"
#define IX(i, j) (int(i)+(N+2)*int(j))

using namespace glm;

class DivergenceTest : public FluidSimFixture {
public:
    DivergenceTest()
            :FluidSimFixture(){}

    void SetUp() override {
        _constants.N = 128;
        FluidSimFixture::SetUp();
    }

    void setVectorField(const std::vector<vec2>& field){
        const auto N = (_constants.N + 2) * (_constants.N + 2);

        auto u = reinterpret_cast<float*>(_u0.map());
        auto v = reinterpret_cast<float*>(_v0.map());
        for(int i = 0; i < N; i++){
            u[i] = field[i].x;
            v[i] = field[i].y;
        }
        _u0.unmap();
        _v0.unmap();
    }
};

TEST_F(DivergenceTest, divergenceOfVectorFieldShouldBeCorrect){

    auto f = [](float x, float y){
        return cos(x) * sin(y);
    };

    auto g = [](float x, float y){
        return sin(x) * sin(y);
    };

    auto h = [](float x, float y){
        return sin(x) * (cos(y) - sin(y));
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

            float dudx = 0.5f * (u[IX(i+1, j)] - u[IX(i-1, j)]) / delta;
            float dudy = 0.5f * (v[IX(i, j+1)] - v[IX(i, j-1)]) / delta;
            float expected = dudx + dudy;

            ASSERT_NEAR(expected, div[IX(i, j)], 0.001);
        }
    }

}