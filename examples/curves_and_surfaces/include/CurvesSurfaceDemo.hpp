#include "VulkanBaseApp.h"

struct Patch{
    VulkanBuffer points;
    VulkanBuffer indices;
};

class CurvesSurfaceDemo : public VulkanBaseApp{
public:
    explicit CurvesSurfaceDemo(const Settings& settings = {});

protected:
    void initApp() override;

    void createDescriptorPool();

    void createDescriptorSetLayouts();

    void updateDescriptorSets();

    void createCommandPool();

    void createPipelineCache();

    void createRenderPipeline();

    void createComputePipeline();

    void initCameras();

    void createPatches();

    void onSwapChainDispose() override;

    void onSwapChainRecreation() override;

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override;

    void renderUI(VkCommandBuffer commandBuffer);

    void tessellationLevelUI(float& outer, float& inner, float& u, float& v);

    void renderBezierCurve(VkCommandBuffer commandBuffer);

    void renderBezierSurface(VkCommandBuffer commandBuffer);

    void renderBezierSurface(VkCommandBuffer commandBuffer, Patch& patch);

    void renderSphereSurface(VkCommandBuffer commandBuffer);

    void renderIcoSphereSurface(VkCommandBuffer commandBuffer);

    void renderCubeSurface(VkCommandBuffer commandBuffer);

    void loadBezierPatches();

    void loadPatch(const std::string& name, Patch& patch);

    void loadTeaCup();

    void createSpherePatch();

    void createIcoSpherePatch();

    void createCubePatch();

    void renderPoints(VkCommandBuffer commandBuffer);

    void update(float time) override;

    void checkAppInputs() override;

    void cleanup() override;

    void onPause() override;


    void movePoint(int pointIndex, glm::vec3 position);

    int isPointOnScreen(glm::vec3 aPoint);

    glm::vec3 selectPoint();

    void createBezierCurvePatch();

protected:
    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } pBezierCurve;

    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
        struct{
            float tessLevelOuter{4};
            float tessLevelInner{4};
            float u{1};
            float v{1};
        } constants;
    } pBezierSurface;

    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
        struct{
            float tessLevelOuter{50};
            float tessLevelInner{50};
            float u{1};
            float v{1};
            float radius{2};
        } constants;
    } pSphereSurface;

    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
        struct{
            float scale[3]{2, 2, 2};
            float tessLevelOuter{100};
            float tessLevelInner{100};
            float u{1};
            float v{1};
            int normalize{0};
        } constants;
    } pCubeSurface;

    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
        struct{
            float tessLevelOuter{4};
            float tessLevelInner{4};
            float u{1};
            float v{1};
            float w{1};
            float radius{2};
        } constants;
    } pIcoSphereSurface;

    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } pPoints;

    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } compute;

    VulkanDescriptorPool descriptorPool;
    VulkanCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    VulkanPipelineCache pipelineCache;
    Patch bezierCurve;
    Patch teapot;
    Patch teacup;
    Patch teaspoon;
    Patch sphere;
    Patch icoSphere;
    Patch cube;
    uint32_t numPoints{4};
    Camera camera2d;
    std::unique_ptr<OrbitingCameraController> cameraController;
    int selectedPoint{-1};
    glm::vec3* pointsHandle = nullptr;
    glm::vec3 mousePosition;
    bool pointSelected{false};
    Action* movePointAction = nullptr;
    float pointRadius = 0.1;
    static constexpr uint32_t maxPoints{6};

    std::array<VulkanBuffer, 10> indexBuffers;

    static constexpr int MODE_CURVES = 0;
    static constexpr int MODE_SURFACE = 1;
    int mode{MODE_CURVES};

    static constexpr int CURVE_BEZIER = 0;
    int curve{CURVE_BEZIER};

    struct {
        VulkanBuffer buffer;
        VulkanDescriptorSetLayout layout;
        VkDescriptorSet descriptorSet;
    } mvp;

    static constexpr int SURFACE_TEAPOT = 1 << 0;
    static constexpr int SURFACE_TEACUP = 1 << 1;
    static constexpr int SURFACE_TEASPOON = 1 << 2;
    static constexpr int SURFACE_PLANE = 1 << 3;
    static constexpr int SURFACE_SPHERE = 1 << 4;
    static constexpr int SURFACE_CUBE = 1 << 5;
    static constexpr int SURFACE_CONE = 1 << 6;
    static constexpr int SURFACE_TORUS = 1 << 7;
    static constexpr int SURFACE_ICO_SPHERE = 1 << 8;
    static constexpr int SURFACE_BEZIER = (SURFACE_TEAPOT | SURFACE_TEASPOON | SURFACE_TEACUP  | SURFACE_PLANE);
    static constexpr VkShaderStageFlags TESSELLATION_SHADER_STAGES_ALL = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    int surface{SURFACE_TEAPOT};

    struct{
        glm::vec3 color{0};
        float width{0.01};
        int enabled{0};
        int solid{1};
    } wireframe;
};