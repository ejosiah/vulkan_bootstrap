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

TEST_F(BoundaryTest, vectorFieldBoundaryCheck){
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
        ASSERT_EQ(0, u[LOC(i, 0)]);
        ASSERT_EQ(0, u[LOC(i, N+1)]);
        ASSERT_EQ(0, v[LOC(0, i)]);
        ASSERT_EQ(0, v[LOC(N+1, i)]);
    }

    context().device.computeCommandPool().oneTimeCommand([&](auto commandBuffer){
        fluidSim.setBoundary(commandBuffer, fluidSim._quantityDescriptorSets[VELOCITY_FIELD_U][0], fluidSim._u[0], HORIZONTAL_COMPONENT_BOUNDARY);
        fluidSim.setBoundary(commandBuffer, fluidSim._quantityDescriptorSets[VELOCITY_FIELD_V][0], fluidSim._v[0], VERTICAL_COMPONENT_BOUNDARY);
    });


    for(int i = 1; i <= N; i++){
        // bottom boundary
        glm::vec2 u0{u[LOC(0, i)], v[LOC(0, i)]};
        glm::vec2 u1{u[LOC(1, i)], v[LOC(1, i)]};

        auto zero = 0.5f * (u0 + u1);
        auto noSlipCondition = glm::all(glm::equal(zero, glm::vec2(0)));
        ASSERT_TRUE(noSlipCondition);

        // top boundary
        u0 = glm::vec2{u[LOC(N+1, i)], v[LOC(N+1, i)]};
        u1 = glm::vec2{u[LOC(N, i)], v[LOC(N, i)]};

        zero = 0.5f * (u0 + u1);
        noSlipCondition = glm::all(glm::equal(zero, glm::vec2(0)));
        ASSERT_TRUE(noSlipCondition);

        // left boundary
        u0 = glm::vec2{u[LOC(i, 0)], v[LOC(i, 0)]};
        u1 = glm::vec2{u[LOC(i, 1)], v[LOC(i, 1)]};

        zero = 0.5f * (u0 + u1);
        noSlipCondition = glm::all(glm::equal(zero, glm::vec2(0)));
        ASSERT_TRUE(noSlipCondition);

        // right boundary
        u0 = glm::vec2{u[LOC(i, N+1)], v[LOC(i, N+1)]};
        u1 = glm::vec2{u[LOC(i, N)], v[LOC(i, N)]};

        zero = 0.5f * (u0 + u1);
        noSlipCondition = glm::all(glm::equal(zero, glm::vec2(0)));
        ASSERT_TRUE(noSlipCondition);
    }

    // bottom left corner
    glm::vec2 u0{u[LOC(0, 0)], v[LOC(0, 0)]};
    glm::vec2 u1{u[LOC(1, 1)], v[LOC(1, 1)]};

    auto zero = 0.5f * (u0 +  u1);
    auto noSlipCondition = glm::all(glm::equal(zero, glm::vec2(0)));
    ASSERT_TRUE(noSlipCondition);

    // bottom right corner
    u0 = glm::vec2{u[LOC(0, N+1)], v[LOC(0, N+1)]};
    u1 = glm::vec2{u[LOC(1, N)], v[LOC(1, N)]};

    zero = 0.5f * (u0 + u1);
    noSlipCondition = glm::all(glm::equal(zero, glm::vec2(0)));
    ASSERT_TRUE(noSlipCondition);

    // top right corner
    u0 = glm::vec2{u[LOC(N+1, N+1)], v[LOC(N+1, N+1)]};
    u1 = glm::vec2{u[LOC(N, N)], v[LOC(N, N)]};

    zero = 0.5f * (u0 + u1);
    noSlipCondition = glm::all(glm::equal(zero, glm::vec2(0)));
    ASSERT_TRUE(noSlipCondition);

    // top left corner
    u0 = glm::vec2{u[LOC(N+1, 0)], v[LOC(N+1, 0)]};
    u1 = glm::vec2{u[LOC(N, 1)], v[LOC(N, 1)]};

    zero = 0.5f * (u0 + u1);
    noSlipCondition = glm::all(glm::equal(zero, glm::vec2(0)));
    ASSERT_TRUE(noSlipCondition);

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

TEST_F(BoundaryTest, DISABLED_boundaryOfConstantVectorFieldShouldAlsoBeConstant){
    const auto N = _constants.N;
    std::vector<vec2> field((N+2) * (N+2));

    for(int j = 1; j <= N; j++){
        for(int i = 1; i <= N; i++){
            vec2 u{1, 0};
            field[LOC(i, j)] = u;
        }
    }

    setVectorField(field);

    auto u = reinterpret_cast<float*>(_u0.map());

    for(int i = 0; i < (N+2); i++){
        ASSERT_EQ(0, u[LOC(0, i)]);
        ASSERT_EQ(0, u[LOC(N+1, i)]);
    }


    context().device.computeCommandPool().oneTimeCommand([&](auto commandBuffer){
        fluidSim.setBoundary(commandBuffer, fluidSim._quantityDescriptorSets[VELOCITY_FIELD_U][0], fluidSim._u[0], HORIZONTAL_COMPONENT_BOUNDARY);
    });

    for(int i = 1; i <= N; i++){
        ASSERT_EQ(1, u[LOC(i, 0)] + u[LOC(i, 1)]);
    }

    ASSERT_EQ(u[LOC(0, 0)]   , 1); // bottom left
    ASSERT_EQ(u[LOC(0, N+1)] , 1);  // bottom right
    ASSERT_EQ(u[LOC(N+1, 0)] , 1);  // top left
    ASSERT_EQ(u[LOC(N+1, N+1)] , 1);   // top right

    _u0.unmap();
}