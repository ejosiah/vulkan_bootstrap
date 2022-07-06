#include "common.h"
#include "vulkan_context.hpp"
#include "ComputePipelins.hpp"
#include "filemanager.hpp"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <spdlog/spdlog.h>
#include <array>
#include <map>
#include <stdexcept>
#include <fmt/format.h>
#include "fluid_sim_gpu.h"

struct Accessor2D{
    float* delegate{};
    int width{};

    struct Column{
        float* delegate;
        int width;
        int j;

        float& operator[](int i) const {
            if(i < 0 || i > width || j < 0 || j > width){
                throw std::out_of_range(fmt::format("index [{}][{}] out of range"));
            }
            auto index = (j+1) * (width + 2) + (i+1);
            return delegate[index];
        }
    };

    Column operator[](int i) const {
        return {delegate, width, i};
    }

    [[nodiscard]]
    float* data() const{
        return delegate;
    }
};


class FluidSimFixture : public ::testing::Test{
public:
    FluidSimFixture(){}
protected:


    void SetUp() override {
        spdlog::set_level(spdlog::level::warn);
        initFileManager();
        auto info = createInfo();
        _context = new VulkanContext{ info };
        _context->init();
        device = &_context->device;
        initBuffer();
        initFluidSim();

    }

    void initFileManager(){
        _fileManager.addSearchPath(".");
        _fileManager.addSearchPath("../../examples/fluid_dynamics");
        _fileManager.addSearchPath("../../examples/fluid_dynamics/spv");
        _fileManager.addSearchPath("../../data/shaders");
        _fileManager.addSearchPath("../../data");
    }

    void initFluidSim() {
        auto dt = _constants.dt;
        fluidSim = FluidSim{ device, _fileManager, _u0, _v0
                , _u, _v, _q0
                , _q, _constants.N, dt, 1.0};
        fluidSim.init();
    }

    void initBuffer(){
        auto n = _constants.N;
        VkDeviceSize size = (n + 2) * (n + 2) * sizeof(float);
        _u0 = device->createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, size, "test_u0_buffer");
        _v0 = device->createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, size, "test_v0_buffer");
        _q0 = device->createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, size, "test_q0_buffer");
        _u = device->createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, size, "test_u_buffer");
        _v = device->createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, size, "test_v_buffer");
        _q = device->createBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, size, "test_q_buffer");
        u0.width = n;
        v0.width = n;
        q0.width = n;
        u.width = n;
        v.width = n;
        q.width = n;
        u0.delegate = reinterpret_cast<float*>( _u0.map() );
        v0.delegate = reinterpret_cast<float*>( _v0.map() );
        q0.delegate = reinterpret_cast<float*>( _q0.map() );
        u.delegate = reinterpret_cast<float*>( _u.map() );
        v.delegate = reinterpret_cast<float*>( _v.map() );
        q.delegate = reinterpret_cast<float*>( _q.map() );
    }

    std::string resource(const std::string& path){
        return _fileManager.getFullPath(path)->string();
    }


    void TearDown() override {
        _u0.unmap();
        _v0.unmap();
        _q0.unmap();
        _u.unmap();
        _v.unmap();
        _q.unmap();
    }

    void advect()  {
        context().device.computeCommandPool().oneTimeCommand([&](auto commandBuffer){
            fluidSim.advect(commandBuffer);
        });
    }


protected:

    VulkanContext& context() const {
        return *_context;
    }

    static ContextCreateInfo createInfo(){
        ContextCreateInfo info{};
        info.applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        info.applicationInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 0);
        info.applicationInfo.pApplicationName = "Fluid Sim Test";
        info.applicationInfo.apiVersion = VK_API_VERSION_1_2;
        info.applicationInfo.pEngineName = "";

        info.instanceExtAndLayers.extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        info.settings.queueFlags = VK_QUEUE_COMPUTE_BIT;

        return info;
    }

    void N(int value){
        _constants.N = value;
    }

    void dt(float value){
        _constants.dt = value;
    }

    void dissipation(float value){
        _constants.dissipation = value;
        fluidSim.dissipation(value);
    }

    struct {
        int N{4};
        float dt{0.0625};
        float dissipation{1};
    } _constants;
    VulkanBuffer _u0;
    VulkanBuffer _v0;
    VulkanBuffer _q0;
    VulkanBuffer _u;
    VulkanBuffer _v;
    VulkanBuffer _q;
    FileManager _fileManager;
    VulkanContext* _context{ nullptr };
    Accessor2D u0, v0, q0, u, v, q;
    std::array<int, 2> sc{_constants.N, _constants.N};
    VulkanDevice* device;
    FluidSim fluidSim;
};