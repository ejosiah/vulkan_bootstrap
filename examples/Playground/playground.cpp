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

const float PI = 3.14159265358979323846264338327950288;
using namespace glm;

using Real = float;

template<size_t N>
using Vector = std::array<float, N>;

template<size_t M, size_t N>
using Matrix = std::array<std::array<float, N>, M>;

template<size_t N>
Vector<N> jacobi(const Matrix<N, N>& A, const Vector<N>& b, int iterations = 99, float tol = 0){
    Vector<N> x0{}, x{}, e{1};
    int k = 0;

    auto converged = [&]{
        return std::all_of(begin(e), end(e), [&](const auto& v){ return v <= tol; });
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
        fmt::print("eps: {}\n", e);
    }

    return x;

}

template<size_t N>
Vector<N> gauss_seidel(const Matrix<N, N>& A, const Vector<N>& b,  int iterations = 99, float tol = 0){
    Vector<N> x0{}, x{}, e{1};
    int k = 0;

    auto converged = [&]{
        return std::all_of(begin(e), end(e), [&](const auto& v){ return v <= tol; });
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
            int value = 0;
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
Matrix<N, N> Identity(){
    Matrix<N, N> M;
    for(int i = 0; i < N; i++){
        for(int j = 0; j < N; j++){
            M[i][j] = float(j == i);
        }
    }
    return M;
}

template<size_t N>
void log(const std::string& title, const Vector<N>& v){

}

int main(){
    float tol = 0;
    int iterations = 10000;

    constexpr int n = 3;
    constexpr int N = n * n;
    float dx = 1.0f/n;
    auto lap = laplacian<N>(1/(dx * dx), true);

    std::array<vec2, N> field{};
    for(int i = 0; i < n; i++){
        for(int j = 0; j < n; j++){
//            float x = 2 * (float(i)/n) - 1;
            float y = 2 * (float(i)/n) - 1;
            int index = i * n + j;
            field[index] = {1, glm::sin(2.0f * pi<float>() * y)};
        }
    }

//    fmt::print("vector field:\n\t");
//    for(int i = 0; i < n; i++){
//        for(int j = 0; j < n; j++){
//            int index = i * n + j;
//            fmt::print("{} ", field[index]);
//        }
//        fmt::print("\n\t");
//    }

    Vector<N> div{};

    for(int i = 0; i < n; i++){
        for(int j = 0; j < n; j++){
            int index = i * n + j;
            int right = min((i+1) * n + j, n-1);
            int left = max((i-1) * n + j, 0);
            int up = min(i * n + (j+1), n-1);
            int down = max(i * n + (j-1), 0);
            auto dudx = 0.5f * n * (field[right].x - field[left].x);
            auto dudy = 0.5f * n * (field[up].y - field[down].y);
            div[index] = dudx + dudy;
        }
    }

//    fmt::print("\ndivergence:\n");
//    for(int i = 0; i < n; i++){
//        for(int j = 0; j < n; j++){
//            int index = i * n + j;
//            fmt::print("\t{} ", div[index]);
//        }
//        fmt::print("\n");
//    }

//    fmt::print("\nlaplacian:\n\t");
//    for(int i = 0; i < N; i++){
//        for(int j = 0; j < N; j++){
//            fmt::print("{} ", lap[i][j]);
//        }
//        fmt::print("\n\t");
//    }

    auto x = jacobi(lap, div, iterations, tol);
    fmt::print("x: {}\n\n", x);

    x = gauss_seidel(lap, div,  iterations, tol);
    fmt::print("x: {}\n\n", x);

    auto precon = Identity<N>();
    x = Vector<N>{};
    auto converged = CG(lap, x, div, precon, iterations, tol);
    if(converged){
        fmt::print("GC converged after {} iterations\n", iterations);
    }else{
        fmt::print("GC  did not converge after {} iterations\n", iterations);
    }
    fmt::print("x: {}\n", x);
}


