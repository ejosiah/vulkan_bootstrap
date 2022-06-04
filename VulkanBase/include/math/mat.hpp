#pragma once

#include <array>
#include <initializer_list>
#include <fmt/format.h>
#include "vecN.hpp"
#include <glm/glm.hpp>

template<size_t M, size_t N>
class mat{
public:
    mat(){
        clear();
    };

    explicit constexpr mat(float value) {
        static_assert(M == N);
        clear();
        for(int m = 0; m < M; m++){
            for(int n = 0; n < N; n++){
                if(n == m){
                    rows[m][n] = value;
                }
            }
        }
    }

    template<size_t MM, size_t NN>
    constexpr mat(std::initializer_list<mat<MM, NN>> list) {
        static_assert(M % MM == 0 && N % NN == 0);
        auto itr = begin(list);

        for(int i = 0; i < list.size(); i++){
            for(int j = 0; j < MM; j++){
                for(int k = 0; k < N; k++){
                    int row = i * MM + j;
                    int col = (j * N + k)%N;
                    if(row == col){
                        auto mat = *itr;
                        rows[row][col] = mat[j][row%NN];
                    }
                }
            }
            std::advance(itr, 1);
        }
    }

    constexpr mat(std::initializer_list<vec<N>> list){
        assert(list.size() == M);
        auto itr = list.begin();
        for(int i = 0; i < M; i++){
            rows[i] = *itr;
            std::advance(itr, 1);
        }
    }

    template<glm::length_t C, glm::length_t R, glm::qualifier Q = glm::defaultp>
    explicit constexpr mat(const glm::mat<C, R, float, Q>& gm) {
        static_assert(C == N && R == M);
        for(auto m = 0; m < M; m++){
            for(auto n = 0; n < N; n++){
                rows[m][n] = gm[n][m];
            }
        }
    }

    constexpr vec<M> operator*(const vec<N>& rhs) const {
        vec<M> v;
        for(int i = 0; i < M; i++){
            v[i] = rhs.dot(rows[i]);
        }
        return v;
    }

    template<size_t MM, size_t NN>
    constexpr mat<M, NN> operator*(const mat<MM, NN>& rhs) const {
        static_assert(N == MM);
        auto tRhs = rhs.transpose();
        mat<M, NN> result;
        for(int m = 0; m < M; m++){
            for(int n = 0; n < NN; n++){
                result[m][n] = rows[m].dot(tRhs.rows[n]);
            }
        }
        return result;
    }

    constexpr mat<M, N> operator*(const float rhs) const {
        mat<M, N> m = *this;
        for(auto& v : m.rows){
            v *= rhs;
        }
        return m;
    }


    constexpr mat<N, M> transpose() const {
        mat<N, M> result;
        for(int m = 0; m < M; m++){
            for(int n = 0; n < N; n++){
                result[n][m] = rows[m][n];
            }
        }
        return result;
    }

    const vec<N>& operator[](const int idx) const {
        return rows[idx];
    }

    vec<N>& operator[](const int idx) {
        return rows[idx];
    }

    void clear() {
        for(auto& v : rows){
            v.clear();
        }
    }

    template<glm::length_t L, glm::qualifier Q = glm::defaultp>
    void set(int row, int offset, const glm::vec<L, float, Q>& v) {
        assert(offset + L <= N);
        switch(L){
            case 2:
                rows[row][offset] = v[0];
                rows[row][offset + 1] = v[1];
                break;
            case 3:
                rows[row][offset] = v[0];
                rows[row][offset + 1] = v[1];
                rows[row][offset + 2] = v[2];
                break;
            case 4:
                rows[row][offset] = v[0];
                rows[row][offset + 1] = v[1];
                rows[row][offset + 2] = v[2];
                rows[row][offset + 3] = v[3];
                break;
            default:
                for(int i = 0; i < L; i++){
                    rows[row][offset + i] = v[i];
                }
                break;
        }
    }

    [[nodiscard]]
    constexpr int numRows() const {
        return M;
    }

    [[nodiscard]]
    constexpr int numColumns() const {
        return N;
    }

    std::array<vec<N>, M> rows{};
};

template<size_t N>
using matN = mat<N, N>;

using mat1 = matN<1>;
using mat3 = matN<3>;
using mat1x12 = mat<1, 12>;
using mat2x12 = mat<2, 12>;
using mat3x12 = mat<3, 12>;
using mat4x12 = mat<4, 12>;
using mat5x12 = mat<5, 12>;
using mat6x12 = mat<6, 12>;
using mat7x12 = mat<7, 12>;
using mat8x12 = mat<8, 12>;
using mat9x12 = mat<9, 12>;
using mat10x12 = mat<10, 12>;
using mat11x12 = mat<11, 12>;
using mat12 = matN<12>;