#include "VulkanBaseApp.h"
#include "BRDFGenerator.hpp"

struct Light{
    glm::vec4 position;

    alignas(16)
    glm::vec3 intensity;
};
struct Material{
    Texture albedo;
    Texture metalness;
    Texture roughness;
    Texture normal;
    Texture ao;
    Texture depth;
    bool invertRoughness{false};
    bool parallaxMapping{false};
};

class PbrDemo : public VulkanBaseApp{
public:
    explicit PbrDemo(const Settings& settings = {});

protected:
    void initApp() override;

    void initCamera();

    void createDescriptorSetLayouts();

    void updateDescriptorSets();

    void updatePbrDescriptors(const Material& material);

    void createUboBuffers();

    void initModel();

    void loadMaterials();

    void createValueSampler();

    void createSRGBSampler();

    void loadEnvironmentMap();

    void createBRDF_LUT();

    void createDescriptorPool();

    void createCommandPool();

    void createPipelineCache();

    void createRenderPipeline();

    void createComputePipeline();

    void renderEnvironment(VkCommandBuffer commandBuffer);

    void drawScreenQuad(VkCommandBuffer commandBuffer);

    void renderModel(VkCommandBuffer commandBuffer);

    void onSwapChainDispose() override;

    void onSwapChainRecreation() override;

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override;

    void update(float time) override;

    void checkAppInputs() override;

    void cleanup() override;

    void onPause() override;

    void renderUI(VkCommandBuffer commandBuffer);

protected:
    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } render;

    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } compute;

    struct {
        VulkanBuffer vertices;
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } screenQuad;

    struct {
        VulkanDescriptorSetLayout setLayout;
        VkDescriptorSet descriptorSet;
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
        Texture texture;
        Texture irradiance;
        Texture specular;
        struct{
            int mapId{0};
            int selection{0};
            float lod{0};
        } constants;
        int numMaps{0};
    } environmentMap;

    Texture brdfLUT;
    Texture placeHolderTexture;

    VulkanDescriptorSetLayout uboSetLayouts;
    VkDescriptorSet uboDescriptorSet;

    struct{
        VulkanDescriptorSetLayout setLayout;
        struct {
            int numLights{6};
            int mapId{0};
            int invertRoughness{0};
            int normalMapping{1};
            int parallaxMapping{0};
            float heightScale{0.1};
        } constants;
        VkDescriptorSet descriptorSet;
    } pbr;

    struct {
        VulkanBuffer vertices;
        VulkanBuffer indices;
    } skybox;

    std::array<Light, 6> lights{{
            {glm::vec4{0, 0, 3, 1}, glm::vec3(23.47, 21.31, 20.79)},
            {glm::vec4{0, 3, 0, 1}, glm::vec3(23.47, 21.31, 20.79)},
            {glm::vec4{3, 0, 0, 1}, glm::vec3(23.47, 21.31, 20.79)},
            {glm::vec4{0, 0, -3, 1}, glm::vec3(23.47, 21.31, 20.79)},
            {glm::vec4{0, -3, 0, 1}, glm::vec3(23.47, 21.31, 20.79)},
            {glm::vec4{-3, 0, 0, 1}, glm::vec3(23.47, 21.31, 20.79)},
    }};

    struct{
        VulkanBuffer lights;
        VulkanBuffer mvp;
    } uboBuffers;

    Action* swapSet = nullptr;

    VulkanDescriptorSetLayout environmentMapSetLayout;
    std::unique_ptr<OrbitingCameraController> cameraController;

    VulkanDescriptorPool descriptorPool;
    VulkanCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    VulkanPipelineCache pipelineCache;
    VkPhysicalDeviceShaderAtomicFloatFeaturesEXT atomicFloatFeatures;
    VulkanDrawable model;
    VulkanSampler valueSampler;
    VulkanSampler sRgbSampler;
    std::array<Material, 9> materials;
    int activeMaterial{0};
    BRDFGenerator brdfGenerator;
    std::map<std::string, std::string> meshMaterialMap;
};