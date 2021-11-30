#include "MeshLoading.h"
#include "glm_format.h"
#include "xforms.h"

MeshLoading::MeshLoading() : VulkanBaseApp("Coordinate Systems"){}

void MeshLoading::initApp() {
    commandPool = device.createCommandPool(*device.queueFamilyIndex.graphics, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    commandBuffers = commandPool.allocate(swapChain.imageCount());
}

void MeshLoading::onSwapChainDispose() {
    dispose(commandPool);
}

void MeshLoading::onSwapChainRecreation() {
    VulkanBaseApp::onSwapChainRecreation();
}

VkCommandBuffer *MeshLoading::buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) {
    auto& commandBuffer = commandBuffers[imageIndex];

    VkCommandBufferBeginInfo beginInfo = initializers::commandBufferBeginInfo();
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkClearValue clearValue{0.0f, 1.0f, 0.0f, 1.0f};

    VkRenderPassBeginInfo beginRenderPass = initializers::renderPassBeginInfo();
    beginRenderPass.renderPass = renderPass;
    beginRenderPass.framebuffer = framebuffers[imageIndex];
    beginRenderPass.renderArea.extent = swapChain.extent;
    beginRenderPass.renderArea.offset = {0, 0};
    beginRenderPass.clearValueCount = 1.0f;
    beginRenderPass.pClearValues = &clearValue;

    vkCmdBeginRenderPass(commandBuffer, &beginRenderPass, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdEndRenderPass(commandBuffer);
    vkEndCommandBuffer(commandBuffer);

    numCommandBuffers = 1;
    return &commandBuffer;

}

void MeshLoading::update(float time) {
    VulkanBaseApp::update(time);
}

void MeshLoading::checkAppInputs() {
    VulkanBaseApp::checkAppInputs();
}


int main(){
//    std::vector<mesh::Mesh> meshes;
//    mesh::load(meshes, "../../data/models/bigship1.obj");
//    fmt::print("SpaceShip\n");
//    for(auto& mesh : meshes){
//        fmt::print("\tmesh: {}\n", mesh.name);
//    }
    mesh::Material m;
    fmt::print("Material: {}\n", sizeof(mesh::Material));
    fmt::print("{}\n", sizeof(mesh::Material) - offsetof(mesh::Material, diffuse));
    fmt::print("name: [offset: {}, size: {}]\n", offsetof(mesh::Material, name), sizeof(m.name));
    fmt::print("diffuse: [offset: {}, size: {}]\n", offsetof(mesh::Material, diffuse), sizeof(m.diffuse));
    fmt::print("ambient: [offset: {}, size: {}]\n", offsetof(mesh::Material, ambient), sizeof(m.ambient));
    fmt::print("specular: [offset: {}, size: {}]\n", offsetof(mesh::Material, specular), sizeof(m.specular));
    fmt::print("emission: [offset: {}, size: {}]\n", offsetof(mesh::Material, emission), sizeof(m.emission));
    fmt::print("transmittance: [offset: {}, size: {}]\n", offsetof(mesh::Material, transmittance), sizeof(m.transmittance));
    fmt::print("shininess: [offset: {}, size: {}]\n", offsetof(mesh::Material, shininess), sizeof(m.shininess));
    fmt::print("ior: [offset: {}, size: {}]\n", offsetof(mesh::Material, ior), sizeof(m.ior));
    fmt::print("opacity: [offset: {}, size: {}]\n", offsetof(mesh::Material, opacity), sizeof(m.opacity));
    fmt::print("illum: [offset: {}, size: {}]\n", offsetof(mesh::Material, illum), sizeof(m.illum));



    return 0;
}