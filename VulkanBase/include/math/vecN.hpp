#pragma once
#include <algorithm>
#include <initializer_list>
#include <array>
#include <vector>
#include <glm/glm.hpp>

template<size_t N>
struct vec{
    std::array<float, N> data{};

    vec() = default;

    template<glm::length_t L, glm::qualifier Q = glm::defaultp>
    constexpr vec(std::initializer_list<glm::vec<L, float, Q>> list) {
        assert(list.size() * L == N);
        auto itr = list.begin();
        for(int i = 0; i < list.size(); i++){
            auto v = *itr;
            for(int j = 0; j < L; j++){
                int idx = i * L + j;
                data[idx] = v[j];
            }
            std::advance(itr, 1);
        }
    }

    template<glm::length_t L, glm::qualifier Q = glm::defaultp>
    constexpr vec(const glm::vec<L, float, Q>& v) {
        static_assert(L == N);
        for(auto i = 0; i < L; i++){
            data[i] = v[i];
        }
    }

    constexpr explicit vec(float value){
        std::fill_n(data.begin(), N, value);
    }

    constexpr vec(std::initializer_list<float> values){
        assert(values.size() == 1 || values.size() == N);
        if(values.size() == 1){
            std::fill_n(data.begin(), N, *values.begin());
        }else {
            auto itr = values.begin();
            for (int i = 0; i < N; i++) {
                data[i] = *itr;
                std::advance(itr, 1);
            }
        }
    }

    const float& operator[](const int idx) const {
        return data[idx];
    }

    float& operator[](const int idx) {
        return data[idx];
    }

    vec<N>& operator*=(float rhs) {
        for(int i = 0; i < N; i++){
            data[i] *= rhs;
        }
        return *this;
    }

    constexpr vec<N> operator*(float rhs) const {
        vec<N> v;
        for(int i = 0; i < N; i++){
            v[i] = data[i] * rhs;
        }
        return v;
    }

    constexpr vec<N> operator+(const vec<N>& rhs) const {
        vec<N> v;
        for(int i = 0; i < N; i++){
            v[i] = data[i] + rhs.data[i];
        }
        return v;
    }

    constexpr vec<N> operator-(const vec<N>& rhs) const {
        vec<N> v;
        for(int i = 0; i < N; i++){
            v[i] = data[i] - rhs[i];
        }
        return v;
    }

    constexpr vec<N>& operator -=(const vec<N>& rhs) {
        for(int i = 0; i < N; i++){
            data[i] -= rhs[i];
        }
        return *this;
    }

    vec<N> operator -() const {
        return (*this) * -1.0f;
    }

    vec<N>& operator +=(const vec<N>& rhs) {
        for(int i = 0; i < N; i++){
            data[i] += rhs[i];
        }
        return *this;
    }

    constexpr float dot(const vec<N>& rhs) const {
        float result = 0;
        for(int i = 0; i < N; i++){
            result += data[i] * rhs[i];
        }
        return result;
    }

    void clear() {
        std::fill_n(data.begin(), N, 0.0f);
    }

    [[nodiscard]]
    constexpr size_t size() const {
        return N;
    }

    template<glm::length_t L>
    std::vector<glm::vec<L, float, glm::defaultp>> split() const {
        static_assert(N % L == 0);
        std::vector<glm::vec<L, float, glm::defaultp>> result;
        int n = N/L;
        for(auto i = 0; i < n; i++){
            glm::vec<L, float, glm::defaultp> v;
            for(auto j = 0; j < L; j++){
                v[j] = data[i * L + j];
            }
            result.push_back(v);
        }
        return result;
    }

    template<glm::length_t L, glm::qualifier Q = glm::defaultp>
    inline void set(int offset, const glm::vec<L, float, Q>& v) {
        assert(offset + L <= N);
        switch(L){
            case 2:
                data[offset] = v[0];
                data[offset + 1] = v[1];
                break;
            case 3:
                data[offset] = v[0];
                data[offset + 1] = v[1];
                data[offset + 2] = v[2];
                break;
            case 4:
                data[offset] = v[0];
                data[offset + 1] = v[1];
                data[offset + 2] = v[2];
                data[offset + 3] = v[3];
                break;
            default:
                for(int i = 0; i < L; i++){
                    data[offset + i] = v[i];
                }
                break;
        }
    }

    auto begin() -> decltype(data.begin()) {
        return data.begin();
    }

    auto end() -> decltype(data.end()) {
        return data.begin();
    }

    auto begin() const -> decltype(data.cbegin())  {
        return data.cbegin();
    }

    auto end() const -> decltype(data.cend())  {
        return data.cbegin();
    }

};

template<size_t N>
constexpr vec<N> clamp(const vec<N>& v, float lower, float upper){
    return {};
}

using vec1 = vec<1>;
using vec2 = vec<2>;
using vec3 = vec<3>;
using vec4 = vec<4>;
using vec12 = vec<12>;