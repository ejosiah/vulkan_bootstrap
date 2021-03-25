#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

inline glm::mat4 ortho(float left, float right, float bottom, float top, float near = -5, float far = 5){
    glm::vec3 T{ -(right + left)/2.0, -(top + bottom)/2.0f, (far + near)/2.0f};
    glm::vec3 S1{ 2.0f/(right - left), 2.0f/(top - bottom), 2.0f/(far - near) };
    glm::vec3 S2{ 1, -1, - 1};

    glm::mat4 proj = glm::scale(glm::mat4(1), S2);
    proj = glm::scale(proj, S1);
    proj = glm::translate(proj, T);
    return proj;
}