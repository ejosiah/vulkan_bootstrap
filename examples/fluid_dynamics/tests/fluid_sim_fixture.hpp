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

#define NOT_YET_IMPLEMENTED FAIL() << "Not yet implemented"

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
        auto path = fs::absolute(".");
        fmt::print("cwd: {}\n\n", path.string());
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
        // TODO install resource files to avoid relative/absolute paths specifications
        _fileManager.addSearchPath(R"(C:\Users\Josiah Ebhomenye\CLionProjects\vulkan_bootstrap\examples\fluid_dynamics\spv)");
        _fileManager.addSearchPath("../../examples/fluid_dynamics");
        _fileManager.addSearchPath("../../examples/fluid_dynamics/spv");
        _fileManager.addSearchPath("../../data/shaders");
        _fileManager.addSearchPath("../../data");
    }

    void initFluidSim() {
        auto dt = _constants.dt;
        fluidSim = FluidSim{ device, _fileManager, _u0, _v0
                , _u, _v, _constants.N, dt, 1.0};
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
        fluidSim._constants.N = value;
    }

    void dt(float value){
        _constants.dt = value;
        fluidSim._constants.dt = value;
    }

    void dissipation(float value){
        _constants.dissipation = value;
        fluidSim.dissipation(value);
    }

    void setVectorField(const std::vector<glm::vec2>& field){
        const auto N = (_constants.N + 2) * (_constants.N + 2);

        auto u = reinterpret_cast<float*>(_u0.map());
        auto v = reinterpret_cast<float*>(_v0.map());
        for(int i = 0; i < N; i++){
            u[i] = field[i].x;
            v[i] = field[i].y;
        }
        _u0.unmap();
        _v0.unmap();
    }

    void setVectorField1(const std::vector<glm::vec2>& field){
        const auto N = (_constants.N + 2) * (_constants.N + 2);

        auto u = reinterpret_cast<float*>(_u.map());
        auto v = reinterpret_cast<float*>(_v.map());
        for(int i = 0; i < N; i++){
            u[i] = field[i].x;
            v[i] = field[i].y;
        }
        _u.unmap();
        _v.unmap();
    }

    void setVectorField(const float* fu, const float* fv){
        const auto N = (_constants.N + 2) * (_constants.N + 2);

        auto u = reinterpret_cast<float*>(_u0.map());
        auto v = reinterpret_cast<float*>(_v0.map());
        for(int i = 0; i < N; i++){
            u[i] = fu[i];
            v[i] = fv[i];
        }
        _u0.unmap();
        _v0.unmap();
    }

    void setScalaField(const std::vector<float>& field){
        const auto N = (_constants.N + 2) * (_constants.N + 2);

        auto f = reinterpret_cast<float*>(fluidSim._pressure.map());
        for(int i = 0; i < N; i++){
            f[i] = field[i];
        }
    }

    void refresh(){
        TearDown();
        initBuffer();
        initFluidSim();
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