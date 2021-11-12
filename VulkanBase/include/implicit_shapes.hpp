#pragma once

namespace imp{

    struct Plane{
        alignas(16) glm::vec3 normal{ 0 };
        float d{ 0 };
    };

    struct Sphere{
        alignas(16) glm::vec3 center{0};
        float radius;
    };

    struct Cylinder{
        alignas(16) glm::vec3 bottom{0};
        alignas(16) glm::vec3 top{0};
        float radius;
    };

    struct Box{
        glm::vec3 min{MAX_FLOAT};
        glm::vec3 max{MIN_FLOAT};
    };

    enum class ImplicitType : uint32_t {
        NONE = 0,
        PLANE = 1,
        SPHERE = 2,
        CYLINDER = 3,
        BOX = 4,
        RECTANGLE = 4
    };


}