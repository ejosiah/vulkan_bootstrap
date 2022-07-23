#include "fluid_sim_fixture.hpp"
#include "fluid_dynamics_cpu.h"
#define LOC(i, j) ((i)*(N+2)+(j))

class ProjectionTest : public FluidSimFixture {
public:
    ProjectionTest()
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

TEST_F(ProjectionTest, produceCorrectDivergenceFreeVelocityField){
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
    
    std::vector<float> u = u0;
    std::vector<float> v = v0;
    std::vector<float> p(size);
    std::vector<float> div(size);

    project0(N, u.data(), v.data(), p.data(), div.data(), iterations);

    context().device.computeCommandPool().oneTimeCommand([&](auto commandBuffer){
        fluidSim.project(commandBuffer, iterations);
    });

    auto& out = fluidSim.v_out;
    auto gpu_u = reinterpret_cast<float*>(fluidSim._u[out].map());
    auto gpu_v = reinterpret_cast<float*>(fluidSim._v[out].map());

    std::fill(begin(div), end(div), 0.0f);
    divergence(N, gpu_u, gpu_v, div.data());

    for(int i = 0; i < (N+2); i++){
        for(int j = 0; j < (N+2); j++){
            ASSERT_NEAR(u[LOC(i, j)], gpu_u[LOC(i, j)], 0.0001);
            ASSERT_NEAR(v[LOC(i, j)], gpu_v[LOC(i, j)], 0.0001);
        }
    }
    fluidSim._u[out].unmap();
    fluidSim._v[out].unmap();
}

TEST_F(ProjectionTest, swapInputAndOutputDescriptorsAfterEveryProjection){
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

    std::vector<float> u = u0;
    std::vector<float> v = v0;
    std::vector<float> p(size);
    std::vector<float> div(size);

    for(int k = 0; k < 10; k++){
        std::fill(begin(p), end(p), 0.0f);
        std::fill(begin(div), end(div), 0.0f);
        project0(N, u.data(), v.data(), p.data(), div.data(), iterations);
        context().device.computeCommandPool().oneTimeCommand([&](auto commandBuffer){
            fluidSim.project(commandBuffer, iterations);
            fluidSim.swapVectorFieldBuffers();
        });

        auto in = fluidSim.v_in;
        auto gpu_u = reinterpret_cast<float*>(fluidSim._u[in].map());
        auto gpu_v = reinterpret_cast<float*>(fluidSim._v[in].map());
        for(int i = 1; i <= N; i++){
            for(int j = 1; j <= N; j++){

                ASSERT_NEAR(u[LOC(i, j)], gpu_u[LOC(i, j)], 0.0001);
                ASSERT_NEAR(v[LOC(i, j)], gpu_v[LOC(i, j)], 0.0001);
            }
        }
        fluidSim._u[in].unmap();
        fluidSim._v[in].unmap();
        fluidSim._divBuffer.unmap();
    }
}