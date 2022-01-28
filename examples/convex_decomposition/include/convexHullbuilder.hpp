#pragma once

#include "ThreadPool.hpp"
#include "oclHelper.h"
#include "Vertex.h"
#include "VulkanDevice.h"
#include "VulkanCommandBuffer.h"
#include <vhacd/VHACD.h>
#include <vector>
#include <future>

struct ConvexHullPoint{
    glm::vec3 position;
    glm::vec3 normal;
};

struct ConvexHulls{
    std::vector<VulkanBuffer> vertices;
    std::vector<VulkanBuffer> indices;
    std::vector<glm::vec4> colors;
    std::vector<float> points;
    std::vector<uint32_t> triangles;
};

struct OpenCLParams{
    int oclAcceleration{1};
    int oclPlatformID{0};
    int oclDeviceID{0};
};


class Callback final : public VHACD::IVHACD::IUserCallback{
public:
    ~Callback() final = default;

    void Update(const double overallProgress, const double stageProgress, const double operationProgress,
                const char *const stage, const char *const operation) final;
};

class LoggingAdaptor final : public VHACD::IVHACD::IUserLogger{
public:
    ~LoggingAdaptor() final = default;

    void Log(const char *const msg) final;
};

class ConvexHullBuilder{
public:
    ConvexHullBuilder() = default;

    ConvexHullBuilder(VulkanDevice* device);

    ConvexHullBuilder& setData(const VulkanBuffer& vertices, const VulkanBuffer& sourceIndexBuffer);

    ConvexHullBuilder& setData(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

    ConvexHullBuilder& setData(const std::vector<float>& points, const std::vector<uint32_t>& indices);

    ConvexHullBuilder& setParams(const VHACD::IVHACD::Parameters& params);

    ConvexHullBuilder& setCallBack(Callback& callback);

    std::future<ConvexHulls> build();

protected:
    void initOpenCL();
    void initVHACD();
    void createCommandPool();
private:
    ConvexHulls m_convexHulls;
    OCLHelper m_oclHelper;
    VHACD::IVHACD* m_interfaceVHACD{nullptr};
    LoggingAdaptor m_loggerVHACD;
    VHACD::IVHACD::Parameters m_params{};
    OpenCLParams m_openClParams{};
    VulkanDevice* m_device{nullptr};
    VulkanCommandPool m_commandPool;
    bool m_openCLOnline = false;
};