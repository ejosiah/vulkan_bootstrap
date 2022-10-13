#include "VulkanRayTraceModel.hpp"
#include "VulkanRayTraceBaseApp.hpp"
#include "shader_binding_table.hpp"
#include "SkyBox.hpp"

 struct Material{
    alignas(16) glm::vec3 albedo{0};
    float metalness{1};
    float roughness{0.5};
    float pad0, pad1;
};

struct GlassMaterial{
    alignas(16) glm::vec3 albedo{0};
    float ior{1.52};
};

enum Brdf{ Cook_Torrance = 0, Glass, Mirror, Count };

enum ShaderIndices {
    eRayGen = 0,
    eMiss,
    eShadowMiss,
    eOcclusionMiss,
    eCookTorranceHit,
    eImplicitIntersect,
    eGlassHit,
    eMirrorHit,
    eOcclusionHit,
    eOcclusionHitAny,
    eGlassOcclusionHit,
    eCheckerboardCallable,
    eFresnelCallable,
    eShaderGroupCount
};

enum HitGroups{ ePrimary = 0, eOcclusion, eNumHitGroups};

class WhittedRayTracer : public VulkanRayTraceBaseApp {
public:
    explicit WhittedRayTracer(const Settings& settings = {});

protected:
    void initApp() override;

    void initCamera();

    void createModels();

    void createPlanes();

    void createSpheres();

    void createSpheres(Brdf brdf, int numSpheres);

    void createSkyBox();

    void createDescriptorPool();

    void createDescriptorSetLayouts();

    void updateDescriptorSets();

    void createCommandPool();

    void createPipelineCache();

    void initCanvas();

    void createInverseCam();

    void createRayTracingPipeline();

    void rayTrace(VkCommandBuffer commandBuffer);

    void rayTraceToCanvasBarrier(VkCommandBuffer commandBuffer) const;

    void CanvasToRayTraceBarrier(VkCommandBuffer commandBuffer) const;

    void onSwapChainDispose() override;

    void onSwapChainRecreation() override;

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override;

    void update(float time) override;

    void checkAppInputs() override;

    void cleanup() override;

    void onPause() override;

protected:
    struct {
        VulkanPipeline pipeline;
        VulkanPipelineLayout layout;
        VulkanDescriptorSetLayout descriptorSetLayout;
        VulkanDescriptorSetLayout instanceDescriptorSetLayout;
        VulkanDescriptorSetLayout vertexDescriptorSetLayout;
        VkDescriptorSet descriptorSet;
        VkDescriptorSet instanceDescriptorSet;
        VkDescriptorSet vertexDescriptorSet;
    } raytrace;

    ShaderTablesDescription shaderTablesDesc;
    ShaderBindingTables bindingTables;

    VulkanBuffer inverseCamProj;
    Canvas canvas{};
    Texture skybox;

    VulkanDescriptorPool descriptorPool;
    VulkanCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    VulkanPipelineCache pipelineCache;
    std::unique_ptr<OrbitingCameraController> camera;

    struct {
        std::vector<imp::Plane> planes;
        VulkanBuffer buffer;
        rt::ImplicitObject asRef;
    } planes;

    struct {
        std::vector<imp::Sphere> spheres;
        VulkanBuffer buffer;
        rt::ImplicitObject asRef;
    } spheres[3];

    VulkanBuffer ctMatBuffer;
    VulkanBuffer glassMatBuffer;
    std::vector<Material> ctMaterials;
    std::vector<GlassMaterial> glassMaterials;

    const int NUM_SPHERES = 100;

};