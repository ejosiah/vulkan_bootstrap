#include "fluid_sim_fixture.hpp"
#define LOC(i, j) ((i)*(N+2)+(j))

using namespace glm;

class BoundaryTest : public FluidSimFixture {
public:
    BoundaryTest()
            : FluidSimFixture() {}

    void SetUp() override {
        _constants.N = 128;
        FluidSimFixture::SetUp();
    }

    static auto f(float x, float y){
        return cos(x) * sin(y);
    };

    static auto g(float x, float y){
        return sin(x) * sin(y);
    };

};

TEST_F(BoundaryTest, vectorFieldHorizontalBoundaryCheck){

    const auto N = _constants.N;
    std::vector<vec2> field((N+2) * (N+2));

    for(int j = 1; j <= N; j++){
        for(int i = 1; i <= N; i++){
            float x = 2.f * static_cast<float>(j)/static_cast<float>(N) - 1;
            float y = 2.f * static_cast<float>(i)/static_cast<float>(N) - 1;

            vec2 u{f(x, y), g(x, y)};
            field[LOC(i, j)] = u;
        }
    }

    setVectorField(field);

    auto u = reinterpret_cast<float*>(_u0.map());
    auto v = reinterpret_cast<float*>(_v0.map());


    for(int i = 0; i < (N+2); i++){
        ASSERT_EQ(0, u[LOC(0, i)]);
        ASSERT_EQ(0, u[LOC(N+1, i)]);

        ASSERT_EQ(0, v[LOC(i, 0)]);
        ASSERT_EQ(0, v[LOC(i, N+1)]);
    }


    context().device.computeCommandPool().oneTimeCommand([&](auto commandBuffer){
        fluidSim.setBoundary(commandBuffer, fluidSim._quantityDescriptorSets[VELOCITY_FIELD_U][0], fluidSim._u[0], HORIZONTAL_BOUNDARY);
    });

    for(int i = 1; i <= N; i++){
        ASSERT_EQ(u[LOC(i, 0)] + u[LOC(i, 1)], 0);
    }

    ASSERT_EQ(u[LOC(0, 0)]   , 0.5 * (u[LOC(1, 0)] + u[LOC(0, 1)])); // bottom left
    ASSERT_EQ(u[LOC(0, N+1)] , 0.5 * (u[LOC(0, N)] + u[LOC(1, N+1)]));  // bottom right
    ASSERT_EQ(u[LOC(N+1, 0)] , 0.5 * (u[LOC(N, 0)] + u[LOC(N+1, 1)]));  // top left
    ASSERT_EQ(u[LOC(N+1, N+1)] , 0.5 * (u[LOC(N, N+1)] + u[LOC(N+1, N)]));   // top right

}

TEST_F(BoundaryTest, vectorFieldVerticalBoundaryCheck){
    const auto N = _constants.N;
    std::vector<vec2> field((N+2) * (N+2));

    for(int j = 1; j <= N; j++){
        for(int i = 1; i <= N; i++){
            float x = 2.f * static_cast<float>(j)/static_cast<float>(N) - 1;
            float y = 2.f * static_cast<float>(i)/static_cast<float>(N) - 1;

            vec2 u{f(x, y), g(x, y)};
            field[LOC(i, j)] = u;
        }
    }

    setVectorField(field);

    auto v = reinterpret_cast<float*>(_v0.map());


    for(int i = 0; i < (N+2); i++){
        ASSERT_EQ(0, v[LOC(i, 0)]);
        ASSERT_EQ(0, v[LOC(i, N+1)]);
    }

    context().device.computeCommandPool().oneTimeCommand([&](auto commandBuffer){
        fluidSim.setBoundary(commandBuffer, fluidSim._quantityDescriptorSets[VELOCITY_FIELD_V][0], fluidSim._v[0], VERTICAL_BOUNDARY);
    });

    for(int i = 1; i <= N; i++){
        ASSERT_EQ(v[LOC(0, i)] + v[LOC(1, i)], 0);
    }


    ASSERT_EQ(v[LOC(0, 0)]        , 0.5 *(v[LOC(1, 0)] + v[LOC(0, 1)]));
    ASSERT_EQ(v[LOC(0, N + 1)]    , 0.5f * (v[LOC(1, N + 1)] + v[LOC(0, N)]));

    ASSERT_EQ(v[LOC(N + 1, 0)]    , 0.5f * (v[LOC(N, 0)] + v[LOC(N + 1, 1)]));
    ASSERT_EQ(v[LOC(N + 1, N + 1)], 0.5f * (v[LOC(N, N + 1)] + v[LOC(N + 1, N)]));
}

TEST_F(BoundaryTest, scalaFieldBoundaryCheck){
    const auto N = _constants.N;
    std::vector<float> field((N+2) * (N+2));

    for(int j = 1; j <= N; j++){
        for(int i = 1; i <= N; i++){
            float x = 2.f * static_cast<float>(j-1)/static_cast<float>(N) - 1;
            float y = 2.f * static_cast<float>(i-1)/static_cast<float>(N) - 1;

            auto z = g(x, y);
            field[LOC(i, j)] = z;
        }
    }


    setScalaField(field);

    auto p = reinterpret_cast<float*>(fluidSim._pressure.map());

    for(int i = 0; i < (N+2); i++){
        ASSERT_EQ(0, p[LOC(i, 0)]);
        ASSERT_EQ(0, p[LOC(i, N+1)]);
        ASSERT_EQ(0, p[LOC(i, 0)]);
        ASSERT_EQ(0, p[LOC(i, N+1)]);
    }

    context().device.computeCommandPool().oneTimeCommand([&](auto commandBuffer){
        fluidSim.setBoundary(commandBuffer, fluidSim._pressureDescriptorSet, fluidSim._pressure, SCALAR_FIELD_BOUNDARY);
    });

    for(int i = 1; i <= N; i++){
        ASSERT_EQ(p[LOC(0, i)], p[LOC(1, i)]);
        ASSERT_EQ(p[LOC(N+1, i)], p[LOC(N, i)]);
        ASSERT_EQ(p[LOC(i, 0)], p[LOC(i, 1)]);
        ASSERT_EQ(p[LOC(i, N+1)], p[LOC(i, N)]);
    }

    ASSERT_EQ(p[LOC(0, 0)]        , 0.5 *(p[LOC(1, 0)] + p[LOC(0, 1)]));
    ASSERT_EQ(p[LOC(0, N + 1)]    , 0.5f * (p[LOC(1, N + 1)] + p[LOC(0, N)]));

    ASSERT_EQ(p[LOC(N + 1, 0)]    , 0.5f * (p[LOC(N, 0)] + p[LOC(N + 1, 1)]));
    ASSERT_EQ(p[LOC(N + 1, N + 1)], 0.5f * (p[LOC(N, N + 1)] + p[LOC(N + 1, N)]));
}