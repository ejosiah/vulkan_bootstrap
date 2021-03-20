#include "VulkanCube.h"

int main() {
    //   spdlog::set_level(spdlog::level::debug);
    try{
        VulkanCubeInstanced app;
        app.run();
    }catch(const std::runtime_error& err){
        spdlog::error(err.what());
    }
    return 0;
}