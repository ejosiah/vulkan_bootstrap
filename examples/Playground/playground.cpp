// Type your code here, or load an example.
#include <glm/glm.hpp>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <glm\glm.hpp>
#include <glm\gtc\packing.hpp>

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "glm_format.h"
#include <array>

#define IX(i, j, N) ((i) * N + (j))

const float PI = 3.14159265358979323846264338327950288;
using namespace glm;

using Real = float;

template<size_t N>
class Vector{
public:
    Vector(float value = 0){
        _data = std::vector<float>(N);
        std::fill(std::begin(_data), std::end(_data), value);
    }

    Vector(float* vPtr){
        _data = std::vector<float>(N);
        std::memcpy(_data.data(), vPtr, sizeof(float) * N);
    }

    float& operator[](int i) const {
        return _data[i];
    }

    auto begin(){
        return std::begin(_data);
    }

    auto end(){
        return std::end(_data);
    }

    auto data(){
        return _data;
    }

private:
    mutable std::vector<float> _data;
};


template<size_t M, size_t N>
class Matrix{
public:
    Matrix(float diag = 0){
        _data = std::vector<float>(M * N);
        for(int i = 0; i < N; i++){
            for(int j = 0; j < N; j++){
                _data[IX(i, j, N)] = float(j == i);
            }
        }
    }

    float* operator[](int i) const {
        assert(i >= 0 && i < M);
        return _data.data() + (i * N);
    }

private:
    mutable std::vector<float> _data;
};

template<size_t N>
float dot(Vector<N> a, Vector<N> b){
    auto res = 0.0f;
    for(int i = 0; i < N; i++){
        res += a[i] * b[i];
    }

    return res;
}

template<size_t M, size_t N>
Vector<M> operator*(const Matrix<M, N>& mat, const Vector<N>& v){
    Vector<M> res{};

    for(int i = 0; i < M; i++){
        Vector<N> row = mat[i];
        res[i] = dot(row, v);
    }
    return res;
}

template<size_t N>
Vector<N> operator*(const Vector<N>& v, float s){
    Vector<N> res = v;

    for(auto& x : res){
        x *= s;
    }
    return res;
}

template<size_t N>
Vector<N> operator+(const Vector<N>& a, const Vector<N>& b){
    Vector<N> c;

    for(int i = 0; i < N; i++){
        c[i] = a[i] + b[i];
    }
    return c;
}

template<size_t N>
Vector<N> operator-(const Vector<N>& a, const Vector<N>& b){
    Vector<N> c;

    for(int i = 0; i < N; i++){
        c[i] = a[i] - b[i];
    }
    return c;
}

template<size_t N>
bool isZero(const Vector<N>& v){
    for(int i = 0; i < N; i++){
        if(v[i] != 0) return false;
    }
    return true;
}

bool isZero(const vec4& v){
    return all(equal(v, vec4(0)));
}

template<size_t N>
float magnitude(const Vector<N>& v){
    return glm::sqrt(dot(v, v));
}

template<size_t N>
float norm(const Vector<N>& v){
    return glm::sqrt(dot(v, v));
}


template<size_t N>
Matrix<N, N> transpose(const Matrix<N, N>& M){
    Matrix<N, N> res{};
    for(int i = 0; i < N; i++){
        for(int j = 0; j < N; j++){
            res[j][i] = M[i][j];
        }
    }
    return res;
}

template<size_t N>
Vector<N> jacobi(const Matrix<N, N>& A, const Vector<N>& b, int iterations = 99, float tol = 0){
    Vector<N> x0{}, x{}, e{1};
    int k = 0;

    auto converged = [&]{
        return std::all_of(e.begin(), e.end(), [&](const auto& v){ return v <= tol; });
    };


    while(!converged()){

        if(k >= iterations) break;
        x0 = x;
        for(int i = 0; i < N; i++){
            x[i] = b[i]/A[i][i];
            for(int j = 0; j < N; j++){
                if(i == j) continue;
                x[i] -= (A[i][j] * x0[j])/A[i][i];
            }
            e[i] = (x[i] - x0[i])/x[i];

        }
        k++;
    }

    if(converged()){
        fmt::print("jacobi converged after {} iterations\n", k);
    }else{
        fmt::print("jacobi did not converge after {} iterations\n", k);
    }

    return x;

}

template<int N>
std::tuple<bool, int> jacobi(const float* b, float* x, float alpha = 1, float beta = 1, int iterations = 99, float tol = 0){
    std::vector<float> e(N * N);
    std::vector<float> x0(N * N);
    int k = 0;

    auto converged = [&]{
        return std::all_of(e.begin(), e.end(), [&](const auto& v){ return v <= tol; });
    };

    while(k < iterations){
        for(int i = 0; i < N; i++){
            for(int j = 0; j < N; j++){
                int index = i * N + j;
                int right = min(i * N + (j+1), N-1);
                int left = max(i * N + (j-1), 0);
                int up = min((i+1) * N + j, N-1);
                int down = max((i-1) * N + j, 0);
                x0[index] = x[index];
                float m = x[left];
                float n = x[right];
                float o = x[up];
                float p = x[down];
                float q = b[index];
//                fmt::print("q => {:.10f}\n", q);
//                x[index = x[IX(i-1, j, N)] + x[IX(i+1, j, N)] + x[IX(i, j-1, N)] + x[IX(i, j+1, N)];
//                x[index += alpha * b[index;
//                x[index /= beta;
                float res = (m + n + o + p + alpha * q)/beta;
                x[index] = res;

                e[index] = (x[index] - x0[index])/x[index];
//                fmt::print("e: {:.10f}\n", e[index]);
            }
        }
        if(converged()) break;
        k++;
    }
    return   std::make_tuple(converged(), k);
}

template<size_t N>
Vector<N> gauss_seidel(const Matrix<N, N>& A, const Vector<N>& b,  int iterations = 99, float tol = 0){
    Vector<N> x0{}, x{}, e{1};
    int k = 0;

    auto converged = [&]{
        return std::all_of(e.begin(), e.end(), [&](const auto& v){ return v <= tol; });
    };


    while(!converged()){

        if(k >= iterations) break;
        for(int i = 0; i < N; i++){
            float oldX = x[i];
            x[i] = b[i]/A[i][i];
            for(int j = 0; j < N; j++){
                if(i == j) continue;
                x[i] -= (A[i][j] * x0[j])/A[i][i];
                x0[i] = x[i];
            }
            e[i] = (x[i] - oldX)/x[i];

        }
        k++;
    }

    if(converged()){
        fmt::print("gauss-seidel converged after {} iterations\n", k);
    }else{
        fmt::print("gauss-seidel did not converge after {} iterations\n", k);
    }

    return x;
}

template<size_t N>
bool CG(const Matrix<N, N> &A, Vector<N> &x, const Vector<N> &b,
                    const Matrix<N, N> &M, int &max_iter, Real &tol){
    Real resid;
    Vector<N> p{}, z{}, q{};
    float alpha, beta, rho, rho_1;

    Real normb = norm(b);
    auto r = b - A*x;

    if (normb == 0.0)
        normb = 1;

    if ((resid = norm(r) / normb) <= tol) {
        tol = resid;
        max_iter = 0;
        return true;
    }

    for (int i = 1; i <= max_iter; i++) {
        z = M * r;
        rho = dot(r, z);

        if (i == 1)
            p = z;
        else {
            beta = rho / rho_1;
            p = z + p * beta;
        }

        q = A*p;
        alpha = rho / dot(p, q);

        x = x + p * alpha;
        r = r - q * alpha;

        if ((resid = norm(r) / normb) <= tol) {
            tol = resid;
            max_iter = i;
            return true;
        }

        rho_1 = rho;
    }

    tol = resid;
    return false;
}


template<size_t N>
Matrix<N, N> laplacian(float s = 1.0f, bool positiveDiag = false){
    Matrix<N, N> M;
    int n = int(sqrt(N));
    for(int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            float value = 0;
            if ((j - 1) == i || (j + 1) == i) {
                value = 1;
            }
            if (j == i) {
//                if ((j % n) - 1 < 0 || (j % n) + 1 >= n) {
//                    value = -1;
//                } else {
//                    value = -2;
//                }
                value = -2;
            }
            value *= s;
            M[i][j] = positiveDiag ? -value : value;
        }
    }
    return M;
}



template<size_t N>
void log(const std::string& title, const Vector<N>& v){

}

template<int N>
vec2 gradient(const float* field, int i, int j){
    int right = min((i+1) * N + j, N-1);
    int left = max((i-1) * N + j, 0);
    int up = min(i * N + (j+1), N-1);
    int down = max(i * N + (j-1), 0);

    auto x = 0.5f * N * (field[right] - field[left]);
    auto y = 0.5f * N * (field[up] - field[down]);
    return vec2{x , y};
}

template<int N>
float divergence(const vec2* field, int i, int j){
    int up = min((i+1) * N + j, N-1);
    int down = max((i-1) * N + j, 0);
    int right = min(i * N + (j+1), N-1);
    int left = max(i * N + (j-1), 0);

    int index = i * N + j;
    auto dudx = 0.5f * N * (sign(right, index) * field[right].x - sign(left, index) * field[left].x);
    auto dudy = 0.5f * N * (sign(up, index) * field[up].y - sign(down, index) * field[down].y);
    return dudx + dudy;
}

float sign(int a, int b){
    return a == b ? -1 : 1;
}

int main(){

    constexpr int n = 8;
    constexpr int N = n * n;
    float dx = 1.0f/n;
    float tol = 0.01;

    int iterations = 50;
    std::array<vec2, N> field{};
    for(int i = 0; i < n; i++){
        for(int j = 0; j < n; j++){
//            float x = 2 * (float(i)/n) - 1;
            float y = 2 * (float(j)/n) - 1;
            int index = i * n + j;
            field[index] = {1, glm::sin(2.0f * pi<float>() * y)};
//            fmt::print("y: {} => {:.10f}\n", y, field[index].y);
        }
    }

    std::vector<float> div(N);
    for(int i = 0; i < n; i++){
        for(int j = 0; j < n; j++){
            int index = i * n + j;
            div[index] = divergence<n>(field.data(), i, j);
        }
    }
    fmt::print("\ndivergence:\n\t");
    for(int i = 0; i < n; i++){
        for(int j = 0; j < n; j++){
            int index = i * n + j;
            fmt::print("{:.10f} ", div[index]);
        }
        fmt::print("\n\t");
    }
    fmt::print("\n\n");


    std::vector<float> p(N);
    float alpha = -dx * dx;
    float beta = 4;

    auto [converged, itr] = jacobi<n>(div.data(), p.data(), alpha, beta, iterations, tol);

    if(converged){
        fmt::print("pressure equation converged after {} iterations\n", itr);
    }else{
        fmt::print("pressure equation did not converge after {} iterations\n", itr);
    }
    for(int i = 0; i < n; i++){
        for(int j = 0; j < n; j++){
            auto x = p[IX(i, j, n)];
            fmt::print("{} ", x);
        }
        fmt::print("\n");
    }
}


