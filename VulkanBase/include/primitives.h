#pragma once

#include "Vertex.h"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_inverse.hpp>
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


    Vertices teapot(glm::mat4 xform = glm::mat4{1}, glm::mat4 lidXform = glm::mat4{1}, const glm::vec4& color = randomColor());

    /**
     * Generates Vertices for a sphere
     * @param rows number of rows on the sphere
     * @param columns number of columns on the sphere
     * @param radius radius of sphere
     * @param color color of sphere
     * @return vertices defining a sphere
     */
    Vertices sphere(int rows, int columns, float radius = 1.0f, glm::mat4 xform = glm::mat4{1}, const glm::vec4& color = randomColor(), VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);

    /**
     *
     * Generates Vertices for a hemisphere
     * @param rows number of rows on the hemisphere
     * @param columns number of columns on the hemisphere
     * @param radius radius of hemisphere
     * @param color color of hemisphere
     * @return vertices defining a hemisphere
     */
    Vertices hemisphere(int rows, int columns, float radius = 1.0f, const glm::vec4& color = randomColor(), VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);

    /**
     * Generates Vertices for a cone
     * @param rows number of rows on the cone
     * @param columns number of columns on the cone
     * @param radius radius of the cone
     * @param height height of the cone
     * @param color color of the cone
     * @return vertices defining a cone
     */
    Vertices cone(int rows, int columns, float radius = 1.0f, float height = 1.0f, const glm::vec4& color = randomColor(), VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);

    /**
     * @brief Generates Vertices for a cylinder
     * @param rows number of rows on the cylinder
     * @param columns number of columns on the cylinder
     * @param radius radius of the cylinder
     * @param height height of the cylinder
     * @param color color of the cylinder
     * @return vertices defining a cylinder
     */
    Vertices cylinder(int rows, int columns, float radius = 1.0f, float height = 1.0f,  const glm::vec4& color = randomColor(), VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);

    Vertices plane(int rows, int columns, float width, float height, const glm::mat4& xform = glm::mat4(1), const glm::vec4& color = randomColor(), VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);

    /**
     * @brief Generates Vertices for a torus
     * @param rows number or rows on the torus
     * @param columns number of columns on the torus
     * @param innerRadius inner radius of the torus
     * @param outerRadius outer radius of the torus
     * @param color color of the torus
     * @return vertices defining a torus
     */
    Vertices torus(int rows, int columns, float innerRadius = 0.5f, float outerRadius = 1.0f, const glm::vec4& color = randomColor(), VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);

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
    Vertices surface(int p,
                     int q,
                     SurfaceFunction&& f,
                     const glm::vec4& color,
                     const glm::mat4& xform = glm::mat4(1),
                     VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);


    Vertices triangleStripToTriangleList(const Vertices& vertices);

    /**
     * @brief normalizes a mesh to a unit version of itself and scales it based on th scaling factor
     * @tparam Mesh Mesh type to normalize, if Mesh is not of type Vertex then it must have a vertices member property
     * of type std::vector<Vertex>
     * @param meshes a vector of meshes to normalize
     * @param scale scaling factor after normalization
     */
    template<typename Mesh>
    inline void normalize(std::vector<Mesh> &meshes, float scale) {
        float radius = 0;
        glm::vec3 center{0};

        auto updateBounds = [&](glm::vec3 position){
            center += position;
            float dSqrd = glm::dot(position, position);
            if(dSqrd > radius){
                radius = dSqrd;
            }
        };

        auto calculateBounds = [&]{
            int numVertices = 0;

            if constexpr (std::is_same_v<Mesh, Vertex>){
                numVertices += meshes.size();
                for (auto &vertex : meshes) {
                    updateBounds(vertex.position.xyz);
                }
            }else {
                for (auto &mesh : meshes) {
                    std::vector<Vertex> &vertices = mesh.vertices;
                    numVertices += vertices.size();
                    for (auto &vertex : vertices) {
                        updateBounds(vertex.position.xyz());
                    }
                }
            }
            radius = std::sqrt(radius);
            center *= (1.0f/float(numVertices));
        };


        auto _scale = [&](float scalingFactor, glm::vec3 offset){
            if constexpr (std::is_same_v<Mesh, Vertex>){
                for (auto &vertex : meshes) {
                    vertex.position.xyz = (vertex.position.xyz + offset) * scalingFactor;
                }
            }else {
                for (auto &mesh : meshes) {
                    std::vector<Vertex> &vertices = mesh.vertices;
                    for (auto &vertex : vertices) {
                          glm::vec3 pos = (vertex.position.xyz() + offset) * scalingFactor;
                          vertex.position = glm::vec4(pos, 1);
                    }
                }
            }
        };

        calculateBounds();
        _scale(scale/radius, -center);
    }

    /**
     * Generates the AABB bounding box of the giving meshes
     * @tparam Mesh Mesh type to generate bounding box for, if Mesh is not of type Vertex then it must have a vertices member property
     * @param meshes Mesh to generate bounding box for
     * @return a tuple of the minimum and maximum vertices of bounding box for the given meshes
     */
    template<typename Mesh>
    inline std::tuple<glm::vec3, glm::vec3> bounds(const std::vector<Mesh>& meshes){
        glm::vec3 min{ std::numeric_limits<float>::max() };
        glm::vec3 max( std::numeric_limits<float>::min() );

        if constexpr (std::is_same_v<Mesh, Vertex>) {
            for (const auto &vertex : meshes) {
                min = glm::min(min, glm::vec3(vertex.position));
                max = glm::max(max, glm::vec3(vertex.position));
            }
        }
//        else{
//           for(const auto& mesh : meshes){
//               const std::vector<Vertices>& vertices = mesh.vertices;
//               for (const auto &vertex : vertices) {
//                   min = glm::min(min, glm::vec3(vertex.position));
//                   max = glm::max(max, glm::vec3(vertex.position));
//               }
//           }
//        }

        return std::make_tuple(min, max);
    }
}
