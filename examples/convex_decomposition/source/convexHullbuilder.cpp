#include "common.h"
#include "random.h"
#include "convexHullbuilder.hpp"

ConvexHullBuilder::ConvexHullBuilder(VulkanDevice *device)
:device{device}{

}

void ConvexHullBuilder::initOpenCL() {
#ifdef CL_VERSION_1_1
    std::vector<std::string> info;

    auto good = oclHelper.GetPlatformsInfo(info, "\t\t");

    if(!good) return;
    std::stringstream ss;
    const auto numPlatforms = info.size();
    ss << "\t Number of OpenCL platforms: " << numPlatforms << "\n";
    for(auto i = 0; i < numPlatforms; i++){
        ss << "\t OpenCL platform [" << i << "]" << "\n";
        ss << info[i];
    }
    ss << "\t Using OpenCL platform [" << openClParams.oclPlatformID << "]" << "\n";
    good = oclHelper.InitPlatform(openClParams.oclPlatformID);
    if(!good) return;

    info.clear();
    good = oclHelper.GetDevicesInfo(info, "\t\t");
    if(!good) return;

    const auto numDevices = info.size();
    ss << "\t Number of OpenCL devices: " << numDevices << "\n";
    for(auto i = 0; i < numDevices; i++){
        ss << "\t OpenCL device [" << i << "]" << "\n";
        ss << info[i];
    }
    good = oclHelper.InitDevice(openClParams.oclDeviceID);

    if(good){
        spdlog::info("OpenCL (ON)");
        spdlog::debug(ss.str());
        spdlog::info("Using OpenCL device [{}]", openClParams.oclDeviceID);
        openCLOnline = true;
    }else{
        openClParams.oclAcceleration = 0;
    }
#else
    spdlog::info("OpenCL (OFF)")
#endif
}

void ConvexHullBuilder::initVHACD() {
    interfaceVHACD = VHACD::CreateVHACD();
#ifdef CL_VERSION_1_1
    if(openClParams.oclAcceleration){
        auto good = interfaceVHACD->OCLInit(oclHelper.GetDevice(), &loggerVHACD);
        if (!good) {
            openClParams.oclAcceleration = 0;
        }
    }
#endif
}


ConvexHullBuilder &ConvexHullBuilder::setData(const VulkanBuffer& sourceVertexBuffer, const VulkanBuffer& sourceindexBuffer) {
    VulkanBuffer vertexBuffer;
    VulkanBuffer indexBuffer;
    
    if(sourceVertexBuffer.mappable){
        vertexBuffer = sourceVertexBuffer;
    }else{
        vertexBuffer = device->createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                VMA_MEMORY_USAGE_CPU_ONLY, vertexBuffer.size);
        
        device->copy(vertexBuffer, vertexBuffer, vertexBuffer.size, 0, 0);
        
    }
    
    if(sourceindexBuffer.mappable){
        indexBuffer = sourceindexBuffer;
    } else{
        indexBuffer = device->createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                          VMA_MEMORY_USAGE_CPU_ONLY, indexBuffer.size);
        device->copy(indexBuffer, indexBuffer, indexBuffer.size, 0, 0);
    }

    auto vertices = reinterpret_cast<Vertex *>(vertexBuffer.map());
    auto numVertices = vertexBuffer.size / sizeof(Vertex);
    convexHulls.points.clear();

    for (int i = 0; i < numVertices; i++) {
        auto &point = vertices[i].position;
        convexHulls.points.push_back(point.x);
        convexHulls.points.push_back(point.y);
        convexHulls.points.push_back(point.z);
    };
    vertexBuffer.unmap();

    auto indices = reinterpret_cast<uint32_t *>(indexBuffer.map());
    auto numIndices = indexBuffer.size / sizeof(uint32_t);

    convexHulls.triangles.clear();
    for (int i = 0; i < numIndices; i++) {
        convexHulls.triangles.push_back(indices[i]);
    }
    indexBuffer.unmap();
    
    return *this;
}

ConvexHullBuilder &
ConvexHullBuilder::setData(const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices) {
    auto numVertices = vertices.size();
    convexHulls.points.clear();

    for (int i = 0; i < numVertices; i++) {
        auto &point = vertices[i].position;
        convexHulls.points.push_back(point.x);
        convexHulls.points.push_back(point.y);
        convexHulls.points.push_back(point.z);
    };

    auto numIndices = vertices.size();

    convexHulls.triangles.clear();
    for (auto i = 0; i < numIndices; i++) {
        convexHulls.triangles.push_back(indices[i]);
    }

    return *this;
}

ConvexHullBuilder &ConvexHullBuilder::setData(const std::vector<float>& points, const std::vector<uint32_t>& indices) {
    convexHulls.points = points;
    convexHulls.triangles = indices;
    return *this;
}

ConvexHullBuilder &ConvexHullBuilder::setParams(const VHACD::IVHACD::Parameters &parameters) {
    params = parameters;
    return *this;
}

ConvexHullBuilder &ConvexHullBuilder::setCallBack(Callback &&callback) {
    callbackVHACD = callback;
    return *this;
}

std::future<par::done> ConvexHullBuilder::build() {
    auto& threadPool = par::ThreadPool::global();

    auto result = threadPool.async([&]{
        auto res = interfaceVHACD->Compute(convexHulls.points.data(), convexHulls.points.size()/3
                , convexHulls.triangles.data(), convexHulls.triangles.size()/3, params );

        if(!res) throw std::runtime_error{"unable to build convex hull"};

        auto numConvexHulls = interfaceVHACD->GetNConvexHulls();

        ConvexHulls hulls{};
        hulls.points = convexHulls.points;
        hulls.triangles = convexHulls.triangles;
        hulls.vertices.reserve(numConvexHulls);

        static uint32_t seed = 1 << 20;
        VHACD::IVHACD::ConvexHull ch{};
        std::vector<std::vector<Vertex>> mesh;
        mesh.reserve(numConvexHulls);

        std::vector<std::vector<uint32_t>> meshIndices;
        meshIndices.reserve(numConvexHulls);

        auto numTriangles = 0u;
        auto totalVertices = 0u;
        auto rnd = rng(0, 1, 1 << 20);
        for(int i = 0; i < numConvexHulls; i++){
            interfaceVHACD->GetConvexHull(i, ch);

            std::vector<Vertex> vertices;
            auto numVertices = ch.m_nPoints * 3;
            for(int j = 0; j < numVertices; j += 3){
                auto x = static_cast<float>(ch.m_points[j + 0]);
                auto y = static_cast<float>(ch.m_points[j + 1]);
                auto z = static_cast<float>(ch.m_points[j + 2]);
                Vertex v{};
                v.position = glm::vec4(x, y, z, 1);
                vertices.push_back(v);
            }

            std::vector<uint32_t> indices;
            numTriangles += ch.m_nTriangles;
            totalVertices += ch.m_nPoints;
            auto numIndices = ch.m_nTriangles * 3;
            for(int j = 0; j < numIndices; j+=3){
                auto i0 = ch.m_triangles[j + 0];
                auto i1 = ch.m_triangles[j + 1];
                auto i2 = ch.m_triangles[j + 2];

                auto& v0 = vertices[i0];
                auto& v1 = vertices[i1];
                auto& v2 = vertices[i2];

                //  generate normals
                glm::vec3 centerCH{ ch.m_center[0], ch.m_center[2], ch.m_center[2]};
                glm::vec3 centerTri = ((v0.position + v1.position + v2.position)/3.0f).xyz();
                auto a = v1.position.xyz() - v0.position.xyz();
                auto b = v2.position.xyz() - v0.position.xyz();
                auto normal = glm::cross(a, b);
                normal = glm::normalize(normal);
                v0.normal = normal;
                v1.normal = normal;
                v2.normal = normal;


                indices.push_back(i0);
                indices.push_back(i1);
                indices.push_back(i2);
            }

            mesh.push_back(vertices);
            meshIndices.push_back(indices);
            hulls.colors.emplace_back(rnd(), rnd(), rnd(), 1.0f);
        }

        std::vector<VulkanBuffer> stagingBuffers;
        stagingBuffers.clear();
        auto numBuffers = numConvexHulls * 2;
        stagingBuffers.reserve(numBuffers);
        for(auto i = 0; i < numConvexHulls; i++){
            auto vertices = mesh[i];
            if(vertices.empty()) continue;

            auto size = BYTE_SIZE(vertices);
            auto stagingBuffer = device->createStagingBuffer(size);
            stagingBuffer.copy(vertices.data(), size);
            stagingBuffers.push_back(stagingBuffer);

            auto vertexBuffer = device->createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
                    , VMA_MEMORY_USAGE_GPU_ONLY, size);
            hulls.vertices.push_back(vertexBuffer);
        }

        for(auto i = 0; i < numConvexHulls; i++){
            auto indices = meshIndices[i];

            if(indices.empty()) continue;

            auto size = BYTE_SIZE(indices);
            auto stagingBuffer = device->createStagingBuffer(size);
            stagingBuffer.copy(indices.data(), size);
            stagingBuffers.push_back(stagingBuffer);

            auto indexBuffer = device->createBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
                    , VMA_MEMORY_USAGE_GPU_ONLY, size);
            hulls.indices.push_back(indexBuffer);
        }

        // for mode (1)  some hulls have no vertices,
        // so we need to make sure we only processing hulls that have  vertices
        numBuffers = stagingBuffers.size();
        numConvexHulls = numBuffers/2;
        commandPoolCH.oneTimeCommands(numBuffers, [&, stagingBuffers = std::move(stagingBuffers)](auto cIndex, auto commandBuffer){
            auto& stagingBuffer = stagingBuffers[cIndex];
            auto index = cIndex % numConvexHulls;

            auto deviceBuffer = (cIndex < numConvexHulls) ? hulls.vertices[index] : hulls.indices[index];

            VkBufferCopy region{0, 0, stagingBuffer.size};
            vkCmdCopyBuffer(commandBuffer, stagingBuffer, deviceBuffer, 1, &region);

            VkBufferMemoryBarrier barrier = initializers::bufferMemoryBarrier();
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
            barrier.srcQueueFamilyIndex = *device->queueFamilyIndex.transfer;
            barrier.dstQueueFamilyIndex = *device->queueFamilyIndex.graphics;
            barrier.buffer = deviceBuffer;
            barrier.offset = 0;
            barrier.size = deviceBuffer.size;

            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT
                    , 0, 0, VK_NULL_HANDLE, 1, &barrier, 0, VK_NULL_HANDLE);
        });

        spdlog::info("Generated {} convex hulls, containing {} triangles, and {} vertices", numConvexHulls, numTriangles, totalVertices);
        return hulls;
    });
    return std::future<par::done>();
}
