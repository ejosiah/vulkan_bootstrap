#include "linear_systems_fixture.hpp"

template<size_t N, size_t K = 20>
std::array<float, N> jacobi(std::array<float, N * N> A, std::array<float, N> b){
    std::array<float, N> x0{}, x{}, epsilon{};

    for(int k = 0; k < K; k++){
        x0 = x;
        for(int i = 0; i < N; i++){
            int ii = i * N + i;
            x[i] = b[i]/A[ii];
            for(int j = 0; j < N; j++){
                if(i == j) continue;
                int ij = i * N + j;
                x[i] = x[i] -  (A[ij]/A[ii] * x0[j]);
            }
            epsilon[i] = (x[i] - x0[i])/x[i];
        }
        fmt::print("error: {}\n", epsilon);
    }
    return x;
}

template<size_t N, size_t K = 20>
std::array<float, N> gauss_seidel(std::array<float, N * N> A, std::array<float, N>& b){
    std::array<float, N> x0{}, x{}, epsilon{};

    for(int k = 0; k < K; k++){
        for(int i = 0; i < N; i++){
            float oldX = x0[i];
            int index = i * N + i;
            auto Aii = A[index];
            x[i] = (b[i] / A[index]);
            for(int j = 0; j < N; j++){
                if(j == i) continue;
                index = i * N + j;
                x[i] = x[i] - ((A[index]/Aii) * x0[j]);
                x0[i] = x[i];
            }
            epsilon[i] = (x[i] - oldX)/x[i];
        }
        fmt::print("solution: {}, error: {}\n", x, epsilon);
    }
    return x;
}

TEST_F(LinearSystemsFixture, jacobiIterationMethod){

    auto _x = jacobi(A, b);
    fmt::print("x: {}\n", _x);

    for(int i = 0; i < N; i++){
        ASSERT_NEAR(x[i], _x[i], 0.001);
    }
}

TEST_F(LinearSystemsFixture, gaussSeidelMethod){

    auto _x = gauss_seidel(A, b);
    fmt::print("x: {}\n", _x);
    for(int i = 0; i < N; i++){
        ASSERT_NEAR(x[i], _x[i], 0.001);
    }
}