#include "MeshLoading.h"
#include "glm_format.h"

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
    std::vector<mesh::Mesh> meshes;
    std::string path = R"(C:\Users\Josiah\OneDrive\media\models\bs_ears.obj)";
    auto start = chrono::high_resolution_clock::now();

    int numVertices = mesh::load(meshes, path);

    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::seconds>(end - start).count();
    spdlog::info("loaded {} vertices in {} seconds from {}\nnum meshes {}", numVertices, duration, path, meshes.size());

    auto& mesh = meshes.front();

    for(int i = 0; i < 10; i++){
        glm::mat3 m(mesh.vertices[i].tangent, mesh.vertices[i].bitangent, mesh.vertices[i].normal);
        auto normal = transpose(m) * mesh.vertices[i].normal;
        fmt::print("{}\n{}\n{}", mesh.vertices[i].normal, mesh.vertices[i].tangent, mesh.vertices[i].bitangent);
        fmt::print("\nnormal: {}\n\n", normal);

    }

    return 0;
}