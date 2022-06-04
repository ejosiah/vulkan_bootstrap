#pragma once

#include "vecN.hpp"
#include "mat.hpp"

namespace lcp {

    constexpr int getMaxIterations(const int n){
        return n < 5 ? 5 : n;
    }

    template<size_t N>
    constexpr vec<N> gaussSeidel(const mat<N, N>& A, const vec<N>& b){
        vec<N> x(0);
        constexpr int maxIterations = getMaxIterations(N);
        for(int iter = 0; iter < maxIterations; iter++){
            for( int i = 0; i < N; i++){
                auto dx = (b[i] - A.rows[i].dot(x)) / A.rows[i][i];
                if( dx * 0.0f == dx * 0.0f){
                    x[i] = x[i] + dx;
                }
            }
        }
        return x;
    }

}