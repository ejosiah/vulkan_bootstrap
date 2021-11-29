#include "common.h"

inline auto rng(float  lowerBound, float  upperBound){
    std::random_device rd;
    std::default_random_engine engine{rd()};

    std::uniform_real_distribution<float> dist{ lowerBound, upperBound };
    return [=] () mutable {
        return dist(engine);
    };
}

inline glm::vec3 randomVec3(const glm::vec3& lower = glm::vec3(-1), const glm::vec3& upper = glm::vec3(1)){
    static auto rngX = rng(lower.x, upper.x);
    static auto rngY = rng(lower.y, upper.y);
    static auto rngZ = rng(lower.z, upper.z);

    return { rngX(), rngY(), rngZ() };
}

inline glm::vec4 randomColor(){
    return glm::vec4(randomVec3(glm::vec3(0)), 1.0f);
}