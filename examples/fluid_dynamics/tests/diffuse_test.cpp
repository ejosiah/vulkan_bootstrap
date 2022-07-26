#include "fluid_sim_fixture.hpp"
#include "fluid_dynamics_cpu.h"
#define LOC(i, j) ((i)*(N+2)+(j))

class DiffuseTest : public FluidSimFixture {
public:
    DiffuseTest()
            :FluidSimFixture(){}

protected:

    void SetUp() override{
        float n = 128;
        N(n);
        dt(1.0f/n);
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

TEST_F(DiffuseTest, diffuseVectorField){
    const auto N = _constants.N;
    int iterations = 80;
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

    float viscosity = 1;
    diffuse0(N, HORIZONTAL_COMPONENT_BOUNDARY, u.data(), u0.data(), viscosity, _constants.dt, iterations);
    diffuse0(N, VERTICAL_COMPONENT_BOUNDARY, v.data(), v0.data(), viscosity, _constants.dt, iterations);

    context().device.computeCommandPool().oneTimeCommand([&](auto commandBuffer){
       fluidSim.diffuse(commandBuffer, VELOCITY_FIELD_U, HORIZONTAL_COMPONENT_BOUNDARY, iterations);
       fluidSim.diffuse(commandBuffer, VELOCITY_FIELD_V, VERTICAL_COMPONENT_BOUNDARY, iterations);
    });

    auto gpu_u = reinterpret_cast<float*>(fluidSim._u[1].map());
    auto gpu_v = reinterpret_cast<float*>(fluidSim._v[1].map());

    for(int i = 0; i < (N+2); i++){
        for(int j = 0; j < (N+2); j++){
            ASSERT_NEAR(u[LOC(i, j)], gpu_u[LOC(i, j)], 0.0001);
            ASSERT_NEAR(v[LOC(i, j)], gpu_v[LOC(i, j)], 0.0001);
        }
    }
    fluidSim._u[1].unmap();
    fluidSim._v[1].unmap();
}

TEST_F(DiffuseTest, diffuseScalarField){
    const auto N = _constants.N;
    int iterations = 80;
    auto size = (N+2) * (N+2);
    std::vector<float> field = createScalarField(N, size);

    std::vector<float> cpu_q0 = field;
    std::vector<float> cpu_q(size);


    float viscosity = 1;
    diffuse0(N, SCALAR_FIELD_BOUNDARY, cpu_q.data(), cpu_q0.data(), viscosity, _constants.dt, iterations);

    VulkanBuffer q0_buffer = context().device.createCpuVisibleBuffer(field.data(), BYTE_SIZE(field), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    VulkanBuffer q_buffer = context().device.createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, BYTE_SIZE(field));

    fluidSim.setQuantity(DENSITY, q0_buffer, q_buffer);

    context().device.computeCommandPool().oneTimeCommand([&](auto commandBuffer){
        fluidSim.diffuse(commandBuffer, DENSITY, SCALAR_FIELD_BOUNDARY, iterations);
    });

    auto gpu_q = reinterpret_cast<float*>(q_buffer.map());

    for(int i = 0; i < (N+2); i++){
        for(int j = 0; j < (N+2); j++){
            ASSERT_NEAR(cpu_q[LOC(i, j)], gpu_q[LOC(i, j)], 0.0001);
        }
    }

    q_buffer.unmap();
    fluidSim._v[1].unmap();
}