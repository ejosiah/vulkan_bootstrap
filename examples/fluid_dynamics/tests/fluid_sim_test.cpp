#include "fluid_sim_fixture.hpp"
#include "fluid_dynamics_cpu.h"
#define LOC(i, j) ((i)*(N+2)+(j))


class FluidSimTest : public FluidSimFixture{
public:
    FluidSimTest(): FluidSimFixture{}
    {}

    void SetUp() override {
        _constants.N = 128;
        _constants.dt = 1.0f/float(_constants.N);
        FluidSimFixture::SetUp();
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
};

TEST_F(FluidSimTest, velocityStep){
    const auto N = _constants.N;
    int iterations = 80;
    float viscosity = 1.0f;
    float dt = _constants.dt;
    auto size = (N+2) * (N+2);
    std::vector<glm::vec2> field = createVectorField(N, size);
    setVectorField(field);

    std::vector<float> u0;
    std::vector<float> v0;
    for(auto & f : field){
        u0.push_back(f.x);
        v0.push_back(f.y);
    }

    std::vector<float> u(size);
    std::vector<float> v(size);

    for (int k = 0; k < 10; k++) {
        vel_step0(N, u.data(), v.data(), u0.data(), v0.data(), viscosity, _constants.dt, iterations);

        context().device.computeCommandPool().oneTimeCommand([&](auto commandBuffer) {
            fluidSim.velocityStep(commandBuffer, dt, viscosity);
        });

        auto gpu_u_buffer = fluidSim.inputBuffer(VELOCITY_FIELD_U);
        auto gpu_v_buffer = fluidSim.inputBuffer(VELOCITY_FIELD_V);

        auto gpu_u = reinterpret_cast<float *>(gpu_u_buffer.map());
        auto gpu_v = reinterpret_cast<float *>(gpu_v_buffer.map());

        for (int i = 0; i < (N + 2); i++) {
            for (int j = 0; j < (N + 2); j++) {
                ASSERT_NEAR(u0[LOC(i, j)], gpu_u[LOC(i, j)], 0.0001);
                ASSERT_NEAR(v0[LOC(i, j)], gpu_v[LOC(i, j)], 0.0001);
            }
        }

        gpu_u_buffer.unmap();
        gpu_v_buffer.unmap();

        u = u0;
        v = v0;
        _u.copy(u.data(), BYTE_SIZE(u), 0);
        _v.copy(v.data(), BYTE_SIZE(u), 0);
        fmt::print("\n");
    }

}