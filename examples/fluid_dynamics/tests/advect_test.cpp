#include "fluid_sim_fixture.hpp"
#include "fluid_dynamics_cpu.h"
#define LOC(i, j) ((i)*(N+2)+(j))

class AdvectTest : public FluidSimFixture {
public:
    AdvectTest()
    :FluidSimFixture(){}

protected:
    void SetUp() override {
        FluidSimFixture::SetUp();
        fluidSim.setQuantity(DENSITY, _q0, _q);
    }

    static auto f(float x, float y){
        return cos(x) * sin(y);
    };

    static auto g(float x, float y){
        return sin(x) * sin(y);
    };

    static std::vector<glm::vec2> createVectorField(int N, int size){
        std::vector<glm::vec2> field(size);
        for(int j = 1; j <= N; j++){
            for(int i = 1; i <= N; i++){
                float x = 2.f * static_cast<float>(j)/static_cast<float>(N) - 1;
                float y = 2.f * static_cast<float>(i)/static_cast<float>(N) - 1;

                glm::vec2 u{f(x, y), g(x, y)};
                field[LOC(i, j)] = u;
            }
        }

        return field;
    }

    static std::vector<float> createScalarField(int N, int size){
        std::vector<float> field(size);

        for(int j = 1; j <= N; j++){
            for(int i = 1; i <= N; i++){
                float x = 2.f * static_cast<float>(j)/static_cast<float>(N) - 1;
                float y = 2.f * static_cast<float>(i)/static_cast<float>(N) - 1;

                field[LOC(i, j)] = g(x, y);
            }
        }

        return field;
    }
};

TEST_F(AdvectTest, advectionShouldProduceTheCorrectQuantity){

    q0[1][1] = 5;
    q0[1][2] = 7;

    q0[2][1] = 12;
    q0[2][2] = 10;

    u0[2][2]  = 1;
    v0[2][2] = 1;


    context().device.computeCommandPool().oneTimeCommand([&](auto commandBuffer){
       fluidSim.advect(commandBuffer, DENSITY, SCALAR_FIELD_BOUNDARY);
    });

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

    context().device.computeCommandPool().oneTimeCommand([&](auto commandBuffer){
        fluidSim.advect(commandBuffer, DENSITY, SCALAR_FIELD_BOUNDARY);
    });

    ASSERT_EQ(9.75, q[2][2]);
}

TEST_F(AdvectTest, advectionShouldProduceCorrectQuantityAndLowerBoundary){
    q0[0][0] = 6;
    q0[0][1] = 12;
    q0[1][0] = 2;
    q0[1][1] = 2;

    u0[1][0]  = 8;
    v0[1][0] = 8;

    context().device.computeCommandPool().oneTimeCommand([&](auto commandBuffer){
        fluidSim.advect(commandBuffer, DENSITY, SCALAR_FIELD_BOUNDARY);
    });
    ASSERT_EQ(1.5, q[1][0]);
}

TEST_F(AdvectTest, advectionShouldProduceCorrectQuantityAndUpperBoundary){
    q0[2][2] = 38;
    q0[2][3] = 55;
    q0[3][2] = 12;
    q0[3][3] = 20;

    u0[3][3]  = -8;
    v0[3][3] = -8;

    context().device.computeCommandPool().oneTimeCommand([&](auto commandBuffer){
        fluidSim.advect(commandBuffer, DENSITY, SCALAR_FIELD_BOUNDARY);
    });

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

    context().device.computeCommandPool().oneTimeCommand([&](auto commandBuffer){
        fluidSim.advect(commandBuffer, DENSITY, SCALAR_FIELD_BOUNDARY);
    });

    ASSERT_EQ(4.75, q[2][2]);
}

TEST_F(AdvectTest, gpuAdvectShouldMatchCpuAdvect){
    N(128);
    dt(1.0f/128.0f);
    const auto N = _constants.N;
    const auto dt = _constants.dt;
    refresh();


    auto size = (N+2) * (N+2);
    std::vector<glm::vec2> vfield = createVectorField(N, size);
    setVectorField(vfield);

    std::vector<float> sfield = createScalarField(N, size);

    std::vector<float> cpu_q0 = sfield;
    std::vector<float> cpu_q(size);

    advect(N, SCALAR_FIELD_BOUNDARY, cpu_q.data(), cpu_q0.data(), u0.data(), v0.data(), dt);

    _q0.copy(sfield.data(), BYTE_SIZE(sfield));
    fluidSim.setQuantity(DENSITY, _q0, _q);

    context().device.computeCommandPool().oneTimeCommand([&](auto commandBuffer){
        fluidSim.advect(commandBuffer, DENSITY, SCALAR_FIELD_BOUNDARY);
    });

    auto gpu_q = reinterpret_cast<float*>(_q.map());
    for(int i = 0; i < (N+2); i++){
        for(int j = 0; j < (N+2); j++){
            ASSERT_NEAR(cpu_q[LOC(i, j)], gpu_q[LOC(i, j)], 0.00001);
        }
    }
    _q.unmap();
}

TEST_F(AdvectTest, advectVelocityField){
    N(8);
    dt(1.0f/8.0f);
    const auto N = _constants.N;
    const auto dt = _constants.dt;
    refresh();

    auto size = (N+2) * (N+2);
    std::vector<glm::vec2> field = createVectorField(N, size);
    setVectorField(field);

    std::vector<float> u0;
    std::vector<float> v0;
    for(auto & f : field){
        u0.push_back(f.x);
        v0.push_back(f.y);
    }

    std::vector<float> cpu_u(size);
    std::vector<float> cpu_v(size);

    advect(N, HORIZONTAL_COMPONENT_BOUNDARY, cpu_u.data(), u0.data(), u0.data(), v0.data(), dt);
    advect(N, VERTICAL_COMPONENT_BOUNDARY, cpu_v.data(), v0.data(), u0.data(), v0.data(), dt);

    context().device.computeCommandPool().oneTimeCommand([&](auto commandBuffer){
        fluidSim.advect(commandBuffer, VELOCITY_FIELD_U, HORIZONTAL_COMPONENT_BOUNDARY);
        fluidSim.advect(commandBuffer, VELOCITY_FIELD_V, VERTICAL_COMPONENT_BOUNDARY);
    });

    auto gpu_u = reinterpret_cast<float*>(_u.map());
    auto gpu_v = reinterpret_cast<float*>(_v.map());

    for(int i = 1; i <= (N+0); i++){
        for(int j = 1; j <= (N+0); j++){
            ASSERT_NEAR(cpu_u[LOC(i, j)], gpu_u[LOC(i, j)], 0.00001);
            ASSERT_NEAR(cpu_v[LOC(i, j)], gpu_v[LOC(i, j)], 0.00001);
        }
    }

    _u.unmap();
    _v.unmap();
}