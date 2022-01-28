#pragma once

#include <memory>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <spdlog/spdlog.h>
#include <algorithm>
#include <vector>
#include <glm/glm.hpp>
#include <boost/functional/hash.hpp>

struct Int128_t{

    uint64_t high, low;

    bool operator==(const Int128_t& rhs) const {
        return this->high == rhs.high && this->low == rhs.low;
    }
};

template<>
struct std::hash<Int128_t>
{
    size_t operator()(Int128_t const& i) const noexcept{
        size_t seed = 0;
        boost::hash_combine(seed, i.high);
        boost::hash_combine(seed, i.low);
        return seed;
    }
};

struct Face;

struct HalfEdge {
    uint32_t vertex{};
    std::shared_ptr<HalfEdge> pair{};
    std::shared_ptr<Face> face{};
    std::shared_ptr<HalfEdge> next{};
    Int128_t id{};
};

struct Face {
    std::shared_ptr<HalfEdge> edge;
};

struct EdgeVertex{
    uint32_t vertex;
    std::shared_ptr<HalfEdge> edge;
};

template<typename point_source, int point_offset = 0, int stride = 3>
struct HalfEdgeMesh{
    std::unordered_map<Int128_t, std::shared_ptr<HalfEdge>> edges;
    std::vector<std::shared_ptr<Face>> faces;
    std::unordered_map<uint32_t, EdgeVertex> vertices;

    using point_type = glm::vec<stride, float, glm::defaultp>;

    HalfEdgeMesh(const uint32_t* indices, size_t size, const point_source* points){
        assert(size % 3 == 0);


        auto hash = [](const point_source& source){
            size_t seed = 0;
            const point_type point = *(reinterpret_cast<const point_type*>((reinterpret_cast<const char*>(&source) + point_offset)));
            for(int j = 0; j < stride; j++){
                boost::hash_combine(seed, point[j]);
            }
            return seed;
        };

        for(int i = 0; i < size; i += 3){
            uint32_t v0 = indices[i + 0];
            uint32_t v1 = indices[i + 1];
            uint32_t v2 = indices[i + 2];

            auto e0 = std::make_shared<HalfEdge>();
            auto e1 = std::make_shared<HalfEdge>();
            auto e2 = std::make_shared<HalfEdge>();

            e0->vertex = v0;
            e1->vertex = v1;
            e2->vertex = v2;

            const auto& point0 = points[v0];
            const auto& point1 = points[v1];
            const auto& point2 = points[v2];

            e0->id =  {hash(point0), hash(point2)};
            e1->id = {hash(point1), hash(point0)};
            e2->id = {hash(point2), hash(point1)};

            e0->next = e1;
            e1->next = e2;
            e2->next = e0;

            auto face = std::make_shared<Face>();
            face->edge = e0;
            e0->face = e1->face = e2->face = face;
            faces.push_back(face);


            vertices.insert(std::make_pair(e0->vertex, EdgeVertex{e0->vertex, e0}));
            vertices.insert(std::make_pair(e1->vertex, EdgeVertex{e1->vertex, e1}));
            vertices.insert(std::make_pair(e2->vertex, EdgeVertex{e2->vertex, e2}));

            edges.insert(std::make_pair(e0->id, e0));
            edges.insert(std::make_pair(e1->id, e1));
            edges.insert(std::make_pair(e2->id, e2));
        }

        // find edge pairs
        uint32_t boundaries = 0;
        for(const auto& face : faces){
            auto edge = face->edge;
            for(int i = 0; i < 3; i++){
                Int128_t id{edge->id.low, edge->id.high};
                auto itr = edges.find(id);
                if (itr != edges.end()) {
                    edge->pair = itr->second;
                    itr->second->pair = edge;
                }
                else {
                    boundaries++;
                }
                edge = edge->next;
            }
        }

        if(edges.size() != size){
            spdlog::warn("Bad mesh:  duplicated edges or inconsistent winding");
        }

        if(boundaries > 0){
            spdlog::warn("Mesh is not watertight. Contains {}  boundary edges", boundaries);
        }
    }

    template<typename Visitor>
    void visitAdjacentEdges(uint32_t vertexId, Visitor&& visit) {
        auto itr = vertices. find(vertexId);
        assert(itr != vertices.end());
        auto vertex = itr->second;
        auto edge = vertex.edge;
        do{
            visit(edge);
            edge = edge->pair->next;
        }while(edge != vertex.edge);
    }
};