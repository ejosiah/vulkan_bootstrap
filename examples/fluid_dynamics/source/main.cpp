#include "FluidDynamicsDemo.hpp"
#include "ImGuiPlugin.hpp"

int main(){
    try{

        Settings settings;
        settings.queueFlags |= VK_QUEUE_COMPUTE_BIT;
        settings.width = settings.height = 1024;
        settings.enabledFeatures.tessellationShader = VK_TRUE;
        settings.enabledFeatures.wideLines = VK_TRUE;
        settings.enabledFeatures.geometryShader = VK_TRUE;
        settings.enabledFeatures.vertexPipelineStoresAndAtomics = VK_TRUE;
        settings.msaaSamples = VK_SAMPLE_COUNT_8_BIT;
        settings.depthTest = true;

        auto app = FluidDynamicsDemo{ settings };
        std::unique_ptr<Plugin> plugin = std::make_unique<ImGuiPlugin>();
        app.addPlugin(plugin);
        app.run();
    }catch(std::runtime_error& err){
        spdlog::error(err.what());
    }
}
