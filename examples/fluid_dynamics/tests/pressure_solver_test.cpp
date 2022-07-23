#include "fluid_sim_fixture.hpp"
#include "fluid_dynamics_cpu.h"
#define LOC(i, j) (int(i) * (N+2) + int(j))

using namespace glm;

class PressureSolverTest : public FluidSimFixture {
public:
    PressureSolverTest()
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

    static std::vector<vec2> createVectorField(int N, int size){
        std::vector<vec2> field(size);
        for(int j = 1; j <= N; j++){
            for(int i = 1; i <= N; i++){
                float x = 2.f * static_cast<float>(j)/static_cast<float>(N) - 1;
                float y = 2.f * static_cast<float>(i)/static_cast<float>(N) - 1;

                vec2 u{f(x, y), g(x, y)};
                field[LOC(i, j)] = u;
            }
        }

        return field;
    }

};

TEST_F(PressureSolverTest, solvePressurePossionEquation){
    const auto N = _constants.N;
    auto size = (N+2) * (N+2);
    std::vector<vec2> field = createVectorField(N, size);
    setVectorField(field);

    std::vector<float> cU;
    std::vector<float> cV;
    for(auto & u : field){
        cU.push_back(u.x);
        cV.push_back(u.y);
    }


    std::vector<float> expectedPressure(size);
    int iterations = 20;

    std::vector<float> div(size);
    divergence(N, cU.data(), cV.data(), div.data());
    pressureSolverJacobi(N, div.data(), expectedPressure.data(), iterations);


    context().device.computeCommandPool().oneTimeCommand([&](auto commandBuffer){
        fluidSim.calculateDivergence(commandBuffer);
    });
    context().device.computeCommandPool().oneTimeCommand([&](auto commandBuffer){
        fluidSim.solvePressureEquation(commandBuffer, iterations);
    });

    auto dx2 = 1.0f/float(N);
    dx2 *= -dx2;
    auto div1 = reinterpret_cast<float*>(fluidSim._divBuffer.map());

    for(int i = 1; i < (N+1); i++){
        for(int j = 1; j < (N+1); j++) {
            ASSERT_NEAR(div[LOC(i, j)],   dx2 * div1[LOC(i, j)], 0.0001);
        }
    }

    auto actualPressure = reinterpret_cast<float*>(fluidSim._pressure.map());
    for(int i = 1; i <= N; i++){
        for(int j = 1; j <= N; j++) {
//            fmt::print("[{}, {}], ", i, j);
            ASSERT_NEAR(expectedPressure[LOC(i, j)], actualPressure[LOC(i, j)], 0.0001);
        }
    }

}

TEST_F(PressureSolverTest, DISABLED_pressureConvergenceTest){
    size_t runs = 10;
    const auto N = _constants.N;
    auto size = (N+2) * (N+2);
    std::vector<float> u(size), v(size), div(size), p(size);

    for(int j = 1; j <= N; j++){
        for(int i = 1; i <= N; i++){
            float x = 2.f * static_cast<float>(j)/static_cast<float>(N) - 1;
            float y = 2.f * static_cast<float>(i)/static_cast<float>(N) - 1;

            u[LOC(i, j)] = f(x, y);
            v[LOC(i, j)] = g(x, y);
        }
    }
    setVectorField(u.data(), v.data());

    divergence(N, u.data(), v.data(), div.data());
    float x0 = 0.0f;
    fmt::print("gauss seidel:\n");
    for(int k = 1; k <= runs; k++ ){
        int iterations = 1 << k;
        std::fill(begin(p), end(p), 0);
        pressureSolver(N, div.data(), p.data(), iterations);
        int i = N/2 + 1;
        auto x = p[LOC(i, i)];
        float epsilon = (x - x0)/x;
        x0 = x;
        fmt::print("\titer: {}, p[{}][{}] => {}, error => {}\n",iterations, k, i, x, epsilon);
    }

    fmt::print("\n\n");
    x0 = 0.0f;
    fmt::print("jacobi:\n");
    for(int k = 1; k <= runs; k++ ){
        int iterations = 1 << k;
        std::fill(begin(p), end(p), 0);
        pressureSolverJacobi(N, div.data(), p.data(), iterations);
        int i = N/2 + 1;
        auto x = p[LOC(i, i)];
        float epsilon = (x - x0)/x;
        x0 = x;
        fmt::print("\titer: {}, p[{}][{}] => {}, error => {}\n",iterations, k, i, x, epsilon);
    }


    context().device.computeCommandPool().oneTimeCommand([&](auto commandBuffer){
        fluidSim.calculateDivergence(commandBuffer);
    });

    x0 = 0.0f;
    fmt::print("\n\n");
    fmt::print("jacobi-gpu:\n");
    for(int k = 0; k <= runs; k++) {
        auto iterations = 1 << k;
        context().device.computeCommandPool().oneTimeCommand([&](auto commandBuffer) {
            vkCmdFillBuffer(commandBuffer, fluidSim._pressure, 0, VK_WHOLE_SIZE, 0);

            VkBufferMemoryBarrier barrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, nullptr
                    , VK_ACCESS_TRANSFER_WRITE_BIT
                    ,(VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT), 0
                    , 0,  fluidSim._pressure, 0, VK_WHOLE_SIZE};
            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0
                    , VK_NULL_HANDLE, 1, &barrier, 0, VK_NULL_HANDLE);
            fluidSim.solvePressureEquation(commandBuffer, iterations);
        });
        auto p = reinterpret_cast<float*>(fluidSim._pressure.map());
        int i = N/2 + 1;
        auto x = p[LOC(i, i)];
        float epsilon = (x - x0)/x;
        x0 = x;
        fmt::print("\titer: {}, p[{}][{}] => {}, error => {}\n",iterations, k, i, x, epsilon);
        fluidSim._pressure.unmap();
    }
}

