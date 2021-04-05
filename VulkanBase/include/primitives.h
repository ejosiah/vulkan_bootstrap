#pragma once

#include "Vertex.h"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include "random.h"

/** @file primitives.h
* @breif collection of primitive generation functions
*/
namespace primitives{

    constexpr uint32_t RESTART_PRIMITIVE = UINT32_MAX;

    /**
     * Generates vertices for cube
     * @param color cube color
     * @return vertices defining a cube
     */
    Vertices cube(const glm::vec4& color = randomColor());

    /**
     * Generates Vertices for a sphere
     * @param rows number of rows on the sphere
     * @param columns number of columns on the sphere
     * @param radius radius of sphere
     * @param color color of sphere
     * @return vertices defining a sphere
     */
    Vertices sphere(int rows, int columns, float radius = 1.0f, const glm::vec4& color = randomColor());

    /**
     *
     * Generates Vertices for a hemisphere
     * @param rows number of rows on the hemisphere
     * @param columns number of columns on the hemisphere
     * @param radius radius of hemisphere
     * @param color color of hemisphere
     * @return vertices defining a hemisphere
     */
    Vertices hemisphere(int rows, int columns, float radius = 1.0f, const glm::vec4& color = randomColor());

    /**
     * Generates Vertices for a cone
     * @param rows number of rows on the cone
     * @param columns number of columns on the cone
     * @param radius radius of the cone
     * @param height height of the cone
     * @param color color of the cone
     * @return vertices defining a cone
     */
    Vertices cone(int rows, int columns, float radius = 1.0f, float height = 1.0f, const glm::vec4& color = randomColor());

    /**
     * Generates Vertices for a cylinder
     * @param rows number of rows on the cylinder
     * @param columns number of columns on the cylinder
     * @param radius radius of the cylinder
     * @param height height of the cylinder
     * @param color color of the cylinder
     * @return vertices defining a cylinder
     */
    Vertices cylinder(int rows, int columns, float radius = 1.0f, float height = 1.0f,  const glm::vec4& color = randomColor());

    Vertices plane(int rows, int columns, float width, float height, const glm::vec4& color = randomColor());

    /**
     * Generates Vertices for a torus
     * @param rows number or rows on the torus
     * @param columns number of columns on the torus
     * @param innerRadius inner radius of the torus
     * @param outerRadius outer radius of the torus
     * @param color color of the torus
     * @return vertices defining a torus
     */
    Vertices torus(int rows, int columns, float innerRadius = 0.5f, float outerRadius = 1.0f, const glm::vec4& color = randomColor());

    /**
     * Generates a parametric surface
     * @tparam SurfaceFunction giving an i,j pair generates a position and normal vector i.e f(i, j) -> std::tuple<glm::vec3, glm::vec3>
     * @param p horizontal sample points
     * @param q vertical sample points
     * @param f giving an i,j pair generates a position and normal vector i.e f(i, j) -> std::tuple<glm::vec3, glm::vec3>
     * @param color surface color
     * @return  returns a surface defined by the parametric function f
     */
    template<typename SurfaceFunction>
    Vertices surface(int p, int q, SurfaceFunction&& f, const glm::vec4& color);

}
