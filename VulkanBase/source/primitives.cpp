#include "primitives.h"

static constexpr auto PI = glm::pi<float>();

Vertices primitives::cube(const glm::vec4& color){
    Vertices mesh;

    mesh.vertices = {

            // FRONT FACE
            {{-0.5, -0.5, 0.5, 1}, color, {0.0f, 0.0f, 1.0f}, {1, 0, 0}, {0, 1, 0},  {0, 0}},
            {{0.5, -0.5, 0.5, 1}, color,  {0.0f, 0.0f, 1.0f}, {1, 0, 0}, {0, 1, 0}, {1, 0}},
            {{0.5, 0.5, 0.5, 1}, color,  {0.0f, 0.0f, 1.0f}, {1, 0, 0}, {0, 1, 0}, {1, 1}},
            {{-0.5, 0.5, 0.5, 1}, color,  {0.0f, 0.0f, 1.0f}, {1, 0, 0}, {0, 1, 0}, {0, 1}},

            // RIGHT FACE
            {{0.5, -0.5, 0.5, 1}, color,  {1.0f, 0.0f, 0.0f}, {0, 0, -1}, {0, 1, 0}, {0, 0}},
            {{0.5, -0.5, -0.5, 1}, color,  {1.0f, 0.0f, 0.0f}, {0, 0, -1}, {0, 1, 0}, {1, 0}},
            {{0.5, 0.5, -0.5, 1}, color,  {1.0f, 0.0f, 0.0f}, {0, 0, -1}, {0, 1, 0}, {1, 1}},
            {{0.5, 0.5, 0.5, 1}, color,  {1.0f, 0.0f, 0.0f}, {0, 0, -1}, {0, 1, 0}, {0, 1}},

            // BACK FAC1
            {{-0.5, -0.5, -0.5, 1}, color, {0.0f, 0.0f, -1.0f}, {-1, 0, 0}, {0, 1, 0}, {1, 0}},
            {{-0.5, 0.5, -0.5, 1}, color, {0.0f, 0.0f, -1.0f}, {-1, 0, 0}, {0, 1, 0}, {1, 1}},
            {{0.5, 0.5, -0.5, 1}, color, {0.0f, 0.0f, -1.0f}, {-1, 0, 0}, {0, 1, 0}, {0, 1}},
            {{0.5, -0.5, -0.5, 1}, color, {0.0f, 0.0f, -1.0f}, {-1, 0, 0}, {0, 1, 0}, {0, 0}},

            // Left face
            {{-0.5, -0.5, 0.5, 1},  color, {-1.0f, 0.0f, 0.0f}, {0, 0, 1}, {0, 1, 0}, {1, 0}},
            {{-0.5, 0.5, 0.5, 1}, color, {-1.0f, 0.0f, 0.0f}, {0, 0, 1}, {0, 1, 0},  {1, 1}},
            {{-0.5, 0.5, -0.5, 1}, color, {-1.0f, 0.0f, 0.0f}, {0, 0, 1}, {0, 1, 0},  {0, 1}},
            {{-0.5, -0.5, -0.5, 1}, color, {-1.0f, 0.0f, 0.0f}, {0, 0, 1}, {0, 1, 0},  {0, 0}},

            // bottom face
            {{-0.5, -0.5, 0.5, 1}, color, {0.0f, -1.0f, 0.0f}, {1, 0, 0}, {0, 0, 1},  {0, 1}},
            {{-0.5, -0.5, -0.5, 1}, color, {0.0f, -1.0f, 0.0f}, {1, 0, 0}, {0, 0, 1},  {0, 0}},
            {{0.5, -0.5, -0.5, 1}, color, {0.0f, -1.0f, 0.0f}, {1, 0, 0}, {0, 0, 1},  {1, 0}},
            {{0.5, -0.5, 0.5, 1}, color, {0.0f, -1.0f, 0.0f}, {1, 0, 0}, {0, 0, 1},  {1, 1}},

            // top face
            {{-0.5, 0.5, 0.5, 1}, color,  {0.0f, 1.0f, 0.0f}, {1, 0, 0}, {0, 0, -1},  {0, 0}},
            {{0.5, 0.5, 0.5, 1}, color,  {0.0f, 1.0f, 0.0f}, {1, 0, 0}, {0, 0, -1},  {1, 0}},
            {{0.5, 0.5, -0.5, 1}, color,  {0.0f, 1.0f, 0.0f}, {1, 0, 0}, {0, 0, -1},  {1, 1}},
            {{-0.5, 0.5, -0.5, 1}, color,  {0.0f, 1.0f, 0.0f}, {1, 0, 0}, {0, 0, -1},  {0, 1}},
    };

    mesh.indices = {
            0,1,2,0,2,3,
            4,5,6,4,6,7,
            8,9,10,8,10,11,
            12,13,14,12,14,15,
            16,17,18,16,18,19,
            20,21,22,20,22,23
    };
    return mesh;
}

Vertices primitives::sphere(int rows, int columns, float radius, glm::mat4 xform, const glm::vec4 &color, VkPrimitiveTopology topology) {
    const auto p = columns;
    const auto q = rows;
    const auto r = radius;

    auto f = [&](float i, float j) {
        float u = 2 * i / p * PI;
        float v = j / q * PI;

        float nx = std::cos(u) * std::sin(v);
        float x = r * nx;

        float ny = std::cos(v);
        float y = r * ny;

        float nz = std::sin(u) * std::sin(v);
        float z = r * nz;

       return std::make_tuple(glm::vec3(x, y, z), glm::vec3(nx, ny, nz));
    };

    return surface(p, q, f, color, xform, topology);
}

Vertices primitives::hemisphere(int rows, int columns, float radius, const glm::vec4 &color, VkPrimitiveTopology topology) {
    auto p = columns;
    auto q = rows;

    auto f = [&](float i, float j) {
        float u = 2 * i / p * PI;
        float v = j / q * PI/2.0f;

        float nx = std::cos(u) * std::sin(v);
        float x = radius * nx;

        float ny = std::cos(v);
        float y = radius * ny;

        float nz = std::sin(u) * std::sin(v);
        float z = radius * nz;

        return std::make_tuple(glm::vec3(x, y, z), glm::vec3(nx, ny, nz));
    };

    return surface(p, q, f, color, glm::mat4{1}, topology);
}

Vertices primitives::cone(int rows, int columns, float radius, float height, const glm::vec4 &color, VkPrimitiveTopology topology) {
    const auto p = columns;
    const auto q = rows;
    const auto h = height;

    auto f = [&](float i, float j) {
        float u = 2 * i / p * PI;
        float v =  j/q * h;

        float nx = std::cos(u);
        float x = radius * v * std::cos(u);

        float ny =  std::sin(u);
        float y = radius  * v * std::sin(u);

        float nz = 0;
        float z =  v - h * 0.5f;

        return std::make_tuple(glm::vec3(x, y, z), glm::vec3(nx, ny, nz));
    };

    return surface(p, q, f, color, glm::mat4{1}, topology);
}

Vertices primitives::cylinder(int rows, int columns, float radius, float height, const glm::vec4 &color, VkPrimitiveTopology topology) {
    const auto p = columns;
    const auto q = rows;
    const auto h = height;

    auto f = [&](float i, float j) {
        float u = (-1.f + 2.f * i/p) * PI;
        float v =   j/q * h;
        float nx = std::sin(u);
        float x = radius *  height * std::sin(u);

        float ny = 0;
        float y = v - h * 0.5f;

        float nz = std::cos(u);
        float z = radius * height * std::cos(u);

        return std::make_tuple(glm::vec3(x, y, z), glm::vec3(nx, ny, nz));
    };

    return surface(p, q, f, color, glm::mat4{1}, topology);
}

Vertices primitives::torus(int rows, int columns, float innerRadius, float outerRadius,  glm::mat4 xform, const glm::vec4 &color, VkPrimitiveTopology topology) {
    auto p = columns;
    auto q = rows;
    auto R = innerRadius;
    auto r = outerRadius;

    auto f = [&](float i, float j) {
        float u = (-1.f + 2.f * i/p) * PI;
        float v =  (-1.f + 2.f * j/q) * PI;

        float x = (R + r * std::cos(v)) * std::cos(u);
        float nx = std::cos(v) * std::cos(u);

        float y = (R + r * std::cos(v)) * std::sin(u);
        float ny = std::cos(v) * sin(u);

        float z = r * std::sin(v);
        float nz =  std::sin(v);

        return std::make_tuple(glm::vec3(x, y, z), glm::vec3(nx, ny, nz));
    };


    return surface(p, q, f, color, xform, topology);
}

Vertices primitives::plane(int rows, int columns, float width, float height, const glm::mat4& xform, const glm::vec4 &color, VkPrimitiveTopology topology) {

    const auto p = columns;
    const auto q = rows;
    const auto halfWidth = width * 0.5f;
    const auto halfHeight = height * 0.5f;

    auto f = [&](float i, float j) {
        float u = i / p * width - halfWidth;
        float v = j / q * height - halfHeight;

        float x = u;
        float nx = 0;

        float y = v;
        float ny = 0;

        float z = 0;
        float nz = 1;


        return std::make_tuple(glm::vec3(x, y, z), glm::vec3(nx, ny, nz));
    };

    return surface(p, q, f, color, xform, topology);
}



template<typename SurfaceFunction>
Vertices primitives::surface(int p, int q, SurfaceFunction &&f, const glm::vec4 &color, const glm::mat4& xform, VkPrimitiveTopology topology) {
    Vertices vertices;
    vertices.topology = topology;
    auto nXform = glm::inverseTranspose(glm::mat3(xform));
    for (int j = 0; j <= q; j++) {
        for (int i = 0; i <= p; i++) {
            auto [position, normal] = f(i, j);
            Vertex vertex{};
            vertex.position = xform * glm::vec4(position, 1.0);
            vertex.normal =  nXform * normal;
            // TODO construct tangents
            vertex.color = color;
            vertex.uv = {float(p - i) / float(p), float(q - j) / float(q)};
            vertices.vertices.push_back(vertex);
        }
    }

    if(topology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST){
        for (int j = 0; j < q; j++) {
            for (int i = 0; i < p; i++) {
                vertices.indices.push_back((j + 1) * (p + 1) + i);
                vertices.indices.push_back(j * (p + 1) + i);
                vertices.indices.push_back((j + 1) * (p + 1) + i + 1);

                vertices.indices.push_back((j + 1) * (p + 1) + i + 1);
                vertices.indices.push_back(j * (p + 1) + i);
                vertices.indices.push_back(j * (p + 1) + i + 1);

            }
        }
    }else {
        for (int j = 0; j < q; j++) {
            for (int i = 0; i <= p; i++) {
                vertices.indices.push_back((j + 1) * (p + 1) + i);
                vertices.indices.push_back(j * (p + 1) + i);
            }
            vertices.indices.push_back(RESTART_PRIMITIVE);
        }
    }

    return vertices;
}

Vertices primitives::triangleStripToTriangleList(const Vertices &vertices) {
    if(vertices.topology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST) return vertices;

    Vertices newVertices{};
    newVertices.vertices = vertices.vertices;
    newVertices.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    if(vertices.indices.empty()){
        assert(vertices.vertices.size() > 2);
        int numFaces = vertices.vertices.size() - 2;

        int i = 0;
        for(; i < numFaces; i++){
            int v0, v1, v2;
            if(i%2 == 0){
                v0 = i;
                v1 = i+1;
                v2 = i+2;
            }else{
                v0 = i+1;
                v1 = i;
                v2 = i+2;
            }
            newVertices.indices.push_back(v0);
            newVertices.indices.push_back(v1);
            newVertices.indices.push_back(v2);
        }
        return newVertices;
    }
    assert(vertices.indices.size() > 2);
    int numFaces = vertices.indices.size() - 2; // we are assuming there are N faces defined by N+2 vertices

    for(auto i = 0, restarts = 0; i < numFaces; i++){
        int v0, v1, v2;
        if((i+restarts)%2 == 0){
            v0 = i;
            v1 = i+1;
            v2 = i+2;
        }else{
            v0 = i+1;
            v1 = i;
            v2 = i+2;
        }
        auto i0 = vertices.indices[v0];
        auto i1 = vertices.indices[v1];
        auto i2 = vertices.indices[v2];

        if(i0 == RESTART_PRIMITIVE || i1 == RESTART_PRIMITIVE || i2 == RESTART_PRIMITIVE){
            restarts++;
            continue;
        }

        newVertices.indices.push_back(i0);
        newVertices.indices.push_back(i1);
        newVertices.indices.push_back(i2);
    }

    return newVertices;
}

Vertices primitives::calculateTangents(Vertices &vertices, bool smooth /* TODO implement smooth tangents */) {
    auto& indices = vertices.indices;

    for(int i = 0; i < indices.size(); i+= 3){
        auto& v0 = vertices.vertices[indices[i]];
        auto& v1 = vertices.vertices[indices[i+1]];
        auto& v2 = vertices.vertices[indices[i+2]];

        auto e1 = v1.position.xyz() - v0.position.xyz();
        auto e2 = v2.position.xyz() - v0.position.xyz();

        auto du1 = v1.uv.x - v0.uv.x;
        auto dv1 = v1.uv.y - v0.uv.y;
        auto du2 = v2.uv.x - v0.uv.x;
        auto dv2 = v2.uv.y - v0.uv.y;

        auto d = 1.f/(du1 * dv2 - dv1 * du2);

        glm::vec3 tn{0};
        tn.x = d * (dv2 * e1.x - dv1 * e2.x);
        tn.y = d * (dv2 * e1.y - dv1 * e2.y);
        tn.z = d * (dv2 * e1.z - dv1 * e2.z);

        glm::vec3 bn{0};
        bn.x = d * (du1 * e2.x - du2 * e1.x);
        bn.y = d * (du1 * e2.y - du2 * e1.y);
        bn.z = d * (du1 * e2.z - du2 * e1.z);

        v0.tangent = tn;
        v1.tangent = tn;
        v2.tangent = tn;

        v0.bitangent = bn;
        v1.bitangent = bn;
        v2.bitangent = bn;
    }

    return vertices;
}