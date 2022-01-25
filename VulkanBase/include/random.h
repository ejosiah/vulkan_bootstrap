#pragma once
#include "common.h"


inline uint32_t randomSeed(){
    static std::random_device rnd;

    return rnd();
}

inline glm::uvec3 randomVec3Seed(){
    return {randomSeed(), randomSeed(), randomSeed()};
}


inline auto rng(float  lowerBound, float  upperBound, uint32_t seed = randomSeed()){
    std::default_random_engine engine{seed};

    std::uniform_real_distribution<float> dist{ lowerBound, upperBound };
    return [=] () mutable {
        return dist(engine);
    };
}

inline glm::vec3 randomVec3(const glm::vec3& lower = glm::vec3(-1), const glm::vec3& upper = glm::vec3(1), glm::uvec3 seed = randomVec3Seed()){
    static auto rngX = rng(lower.x, upper.x, seed.x);
    static auto rngY = rng(lower.y, upper.y, seed.y);
    static auto rngZ = rng(lower.z, upper.z, seed.z);

    return { rngX(), rngY(), rngZ() };
}

//inline glm::vec3 randomVec3(float lower, float upper){
//    return randomVec3(glm::vec3(lower), glm::vec3(upper));
//}

inline glm::vec4 randomColor(glm::uvec3 seed = randomVec3Seed()){
    return glm::vec4(randomVec3(glm::vec3(0), glm::vec3(1), seed), 1.0f);
}