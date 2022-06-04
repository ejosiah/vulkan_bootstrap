#include "common.h"
#include "random.h"
#include "convexHullbuilder.hpp"

ConvexHullBuilder::ConvexHullBuilder(VulkanDevice *device)
: m_device{device}{
    initOpenCL();
    initVHACD();
    createCommandPool();
}

void ConvexHullBuilder::initOpenCL() {
#ifdef CL_VERSION_1_1
    std::vector<std::string> info;

    auto good = m_oclHelper.GetPlatformsInfo(info, "\t\t");

    if(!good) return;
    std::stringstream ss;
    const auto numPlatforms = info.size();
    ss << "\t Number of OpenCL platforms: " << numPlatforms << "\n";
    for(auto i = 0; i < numPlatforms; i++){
        ss << "\t OpenCL platform [" << i << "]" << "\n";
        ss << info[i];
    }
    ss << "\t Using OpenCL platform [" << m_openClParams.oclPlatformID << "]" << "\n";
    good = m_oclHelper.InitPlatform(m_openClParams.oclPlatformID);
    if(!good) return;

    info.clear();
    good = m_oclHelper.GetDevicesInfo(info, "\t\t");
    if(!good) return;

    const auto numDevices = info.size();
    ss << "\t Number of OpenCL devices: " << numDevices << "\n";
    for(auto i = 0; i < numDevices; i++){
        ss << "\t OpenCL device [" << i << "]" << "\n";
        ss << info[i];
    }
    good = m_oclHelper.InitDevice(m_openClParams.oclDeviceID);

    if(good){
        spdlog::info("OpenCL (ON)");
        spdlog::debug(ss.str());
        spdlog::info("Using OpenCL device [{}]", m_openClParams.oclDeviceID);
        m_openCLOnline = true;
    }else{
        m_openClParams.oclAcceleration = 0;
    }
#else
    spdlog::info("OpenCL (OFF)");
#endif
}

void ConvexHullBuilder::initVHACD() {
    m_interfaceVHACD = VHACD::CreateVHACD();
#ifdef CL_VERSION_1_1
    if(m_openClParams.oclAcceleration){
        auto good = m_interfaceVHACD->OCLInit(m_oclHelper.GetDevice(), &m_loggerVHACD);
        if (!good) {
            m_openClParams.oclAcceleration = 0;
        }
    }
#endif
}

void ConvexHullBuilder::createCommandPool() {
    auto queueFamily = m_device->queueFamilyIndex.transfer.has_value()
                       ? m_device->queueFamilyIndex.transfer : m_device->queueFamilyIndex.graphics;
    m_commandPool = m_device->createCommandPool(*queueFamily, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
}

ConvexHullBuilder &ConvexHullBuilder::setData(const VulkanBuffer& sourceVertexBuffer, const VulkanBuffer& sourceIndexBuffer) {
    VulkanBuffer vertexBuffer;
    VulkanBuffer indexBuffer;
    
    if(sourceVertexBuffer.mappable){
        vertexBuffer = sourceVertexBuffer;
    }else{
        vertexBuffer = m_device->createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                              VMA_MEMORY_USAGE_CPU_ONLY, sourceVertexBuffer.size);
        
        m_device->copy(sourceVertexBuffer, vertexBuffer, vertexBuffer.size, 0, 0);
        
    }
    
    if(sourceIndexBuffer.mappable){
        indexBuffer = sourceIndexBuffer;
    } else{
        indexBuffer = m_device->createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                             VMA_MEMORY_USAGE_CPU_ONLY, sourceIndexBuffer.size);
        m_device->copy(sourceIndexBuffer, indexBuffer, indexBuffer.size, 0, 0);
    }

    auto vertices = reinterpret_cast<Vertex *>(vertexBuffer.map());
    auto numVertices = vertexBuffer.size / sizeof(Vertex);
    m_convexHulls.points.clear();

    for (int i = 0; i < numVertices; i++) {
        auto &point = vertices[i].position;
        m_convexHulls.points.push_back(point.x);
        m_convexHulls.points.push_back(point.y);
        m_convexHulls.points.push_back(point.z);
    };
    vertexBuffer.unmap();

    auto indices = reinterpret_cast<uint32_t *>(indexBuffer.map());
    auto numIndices = indexBuffer.size / sizeof(uint32_t);

    m_convexHulls.triangles.clear();
    for (int i = 0; i < numIndices; i++) {
        m_convexHulls.triangles.push_back(indices[i]);
    }
    indexBuffer.unmap();
    
    return *this;
}

ConvexHullBuilder &
ConvexHullBuilder::setData(const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices) {
    auto numVertices = vertices.size();
    m_convexHulls.points.clear();

    for (int i = 0; i < numVertices; i++) {
        auto &point = vertices[i].position;
        m_convexHulls.points.push_back(point.x);
        m_convexHulls.points.push_back(point.y);
        m_convexHulls.points.push_back(point.z);
    };

    auto numIndices = vertices.size();

    m_convexHulls.triangles.clear();
    for (auto i = 0; i < numIndices; i++) {
        m_convexHulls.triangles.push_back(indices[i]);
    }

    return *this;
}

ConvexHullBuilder &ConvexHullBuilder::setData(const std::vector<float>& points, const std::vector<uint32_t>& indices) {
    m_convexHulls.points = points;
    m_convexHulls.triangles = indices;
    return *this;
}

ConvexHullBuilder &ConvexHullBuilder::setParams(const VHACD::IVHACD::Parameters &parameters) {
    m_params = parameters;
    return *this;
}

ConvexHullBuilder &ConvexHullBuilder::setCallBack(Callback &callback) {
    m_params.m_callback = &callback;
    return *this;
}

std::future<ConvexHulls> ConvexHullBuilder::build() {
    return par::ThreadPool::global().async([&]{
        auto res = m_interfaceVHACD->Compute(m_convexHulls.points.data(), m_convexHulls.points.size() / 3
                , m_convexHulls.triangles.data(), m_convexHulls.triangles.size() / 3, m_params );

        if(!res) throw std::runtime_error{"unable to build convex hull"};

        auto numConvexHulls = m_interfaceVHACD->GetNConvexHulls();

        ConvexHulls hulls{};
        hulls.points = m_convexHulls.points;
        hulls.triangles = m_convexHulls.triangles;
        hulls.vertices.reserve(numConvexHulls);

        static uint32_t seed = 1 << 20;
        VHACD::IVHACD::ConvexHull ch{};
        std::vector<std::vector<ConvexHullPoint>> mesh;
        mesh.reserve(numConvexHulls);

        std::vector<std::vector<uint32_t>> meshIndices;
        meshIndices.reserve(numConvexHulls);

        auto numTriangles = 0u;
        auto totalVertices = 0u;
        auto rnd = rng(0, 1, 1 << 20);
        for(int i = 0; i < numConvexHulls; i++){
            m_interfaceVHACD->GetConvexHull(i, ch);

            std::vector<ConvexHullPoint> vertices;
            auto numVertices = ch.m_nPoints * 3;
            for(int j = 0; j < numVertices; j += 3){
                auto x = static_cast<float>(ch.m_points[j + 0]);
                auto y = static_cast<float>(ch.m_points[j + 1]);
                auto z = static_cast<float>(ch.m_points[j + 2]);
                ConvexHullPoint p{};
                p.position = glm::vec3(x, y, z);
                vertices.push_back(p);
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
                glm::vec3 centerTri = (v0.position + v1.position + v2.position)/3.0f;
                auto a = v1.position - v0.position;
                auto b = v2.position - v0.position;
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
            auto stagingBuffer = m_device->createStagingBuffer(size);
            stagingBuffer.copy(vertices.data(), size);
            stagingBuffers.push_back(stagingBuffer);

            auto vertexBuffer = m_device->createBuffer(
                    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
                    | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
                    , VMA_MEMORY_USAGE_CPU_TO_GPU, size);
            hulls.vertices.push_back(vertexBuffer);
        }

        for(auto i = 0; i < numConvexHulls; i++){
            auto indices = meshIndices[i];

            if(indices.empty()) continue;

            auto size = BYTE_SIZE(indices);
            auto stagingBuffer = m_device->createStagingBuffer(size);
            stagingBuffer.copy(indices.data(), size);
            stagingBuffers.push_back(stagingBuffer);

            auto indexBuffer = m_device->createBuffer(
                    VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
                    | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
                    , VMA_MEMORY_USAGE_CPU_TO_GPU, size);
            hulls.indices.push_back(indexBuffer);
        }

        // for mode (1)  some hulls have no vertices,
        // so we need to make sure we are only processing hulls that have  vertices
        numBuffers = stagingBuffers.size();
        numConvexHulls = numBuffers/2;
        m_commandPool.oneTimeCommands(numBuffers, [&, stagingBuffers = std::move(stagingBuffers)](auto cIndex, auto commandBuffer){
            auto& stagingBuffer = stagingBuffers[cIndex];
            auto index = cIndex % numConvexHulls;

            auto deviceBuffer = (cIndex < numConvexHulls) ? hulls.vertices[index] : hulls.indices[index];

            VkBufferCopy region{0, 0, stagingBuffer.size};
            vkCmdCopyBuffer(commandBuffer, stagingBuffer, deviceBuffer, 1, &region);

            VkBufferMemoryBarrier barrier = initializers::bufferMemoryBarrier();
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
            barrier.srcQueueFamilyIndex = *m_device->queueFamilyIndex.transfer;
            barrier.dstQueueFamilyIndex = *m_device->queueFamilyIndex.graphics;
            barrier.buffer = deviceBuffer;
            barrier.offset = 0;
            barrier.size = deviceBuffer.size;

            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
                    , 0, 0, VK_NULL_HANDLE, 1, &barrier, 0, VK_NULL_HANDLE);
        });

        spdlog::info("Generated {} convex hulls, containing {} triangles, and {} vertices", numConvexHulls, numTriangles, totalVertices);
        return hulls;
    });
}

ConvexHullBuilder &ConvexHullBuilder::maxNumVerticesPerCH(int value) {
    m_params.m_maxNumVerticesPerCH = value;
    return *this;
}


void LoggingAdaptor::Log(const char *const msg) {
    spdlog::info(msg);
}


void Callback::Update(const double overallProgress, const double stageProgress, const double operationProgress,
                      const char *const stage, const char *const operation) {
}