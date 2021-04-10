#pragma once

static constexpr auto DELTA = 0.0001f;

template<typename F>
inline float dfdx(F&& f, float x){
    return (f(x + DELTA) - f(x))/DELTA;
}

template<typename F>
inline float dfdx(F&& f, float x, float y){
    return (f(x + DELTA, y) - f(x, y))/DELTA;
}

template<typename F>
inline float dfdy(F&& f, float x, float y){
    return (f(x, y + DELTA) - f(x, y))/DELTA;
}


template<typename F>
inline float dfdx(F&& f, float x, float y, float z){
    return (f(x + DELTA, y, z) - f(x, y, z))/DELTA;
}

template<typename F>
inline float dfdy(F&& f, float x, float y, float z){
    return (f(x, y + DELTA, z) - f(x, y, z))/DELTA;
}
template<typename F>
inline float dfdz(F&& f, float x, float y, float z){
    return  (f(x, y, z + DELTA) - f(x, y, z))/DELTA;
}