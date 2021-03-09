#include <iostream>
#include "../include/VulkanBaseApp.h"


int main() {
 //   spdlog::set_level(spdlog::level::debug);
   try{
        VulkanBaseApp app{"Boot strap"};
        app.run();
    }catch(const std::runtime_error& err){
        spdlog::error(err.what());
    }
    return 0;
}
