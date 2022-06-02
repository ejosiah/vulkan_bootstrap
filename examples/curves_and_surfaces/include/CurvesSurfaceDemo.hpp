#include "VulkanBaseApp.h"
#include <imgui.h>
struct Patch{
    VulkanBuffer points;
    VulkanBuffer indices;
    int numPoints{0};
    int numIndices{0};
};

class CurvesSurfaceDemo : public VulkanBaseApp{
public:
    explicit CurvesSurfaceDemo(const Settings& settings = {});

protected:
    void initApp() override;

    void createDescriptorPool();

    void createDescriptorSetLayouts();

    void createDescriptorSets();

    void refreshDescriptorSets();

    void updateDescriptorSets();

    void createCommandPool();

    void createPipelineCache();

    void createRenderPipeline();

    void createComputePipeline();

    void initCameras();

    void updateCameras();

    void initUniforms();

    void createPatches();

    void onSwapChainDispose() override;

    void onSwapChainRecreation() override;

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override;

    void renderUI(VkCommandBuffer commandBuffer);

    void tessellationLevelUI(float& outer, float& inner, float& u, float& v);

    void renderCurve(VkCommandBuffer commandBuffer);

    void renderCurve(VkCommandBuffer commandBuffer, VulkanPipeline& pipeline, VulkanPipelineLayout& layout, Patch& patch, void* constants = nullptr, uint32_t constantsSize = 0, VkShaderStageFlags shaderStageFlags = 0, int numIndices = 0);

    void renderBezierSurface(VkCommandBuffer commandBuffer);

    void renderBezierSurface(VkCommandBuffer commandBuffer, Patch& patch);

    void renderSphereSurface(VkCommandBuffer commandBuffer);

    void renderTorusSurface(VkCommandBuffer commandBuffer);

    void renderCylinderSurface(VkCommandBuffer commandBuffer);

    void renderCone(VkCommandBuffer commandBuffer);

    void renderIcoSphereSurface(VkCommandBuffer commandBuffer);

    void renderCubeSurface(VkCommandBuffer commandBuffer);

    void renderSweptSurface(VkCommandBuffer commandBuffer);

    void loadBezierPatches();

    void loadPatch(const std::string& name, Patch& patch);

    void loadTexture();

    void createSpherePatch();

    void createIcoSpherePatch();

    void createCubePatch();

    void createHermitePatch();

    void renderControlPoints(VkCommandBuffer commandBuffer,  Patch& patch);

    void renderTangentLines(VkCommandBuffer commandBuffer, Patch& patch, VulkanBuffer* indices = nullptr, uint32_t indexCount = 0);

    void update(float time) override;

    void checkAppInputs() override;

    void cleanup() override;

    void onPause() override;

    void updatePointHandle(Patch& patch);

    void movePoint(int pointIndex, glm::vec3 position);

    bool isPointOnScreen(glm::vec3 aPoint, int& position);

    glm::vec3 selectPoint();

    void createBezierCurvePatch();

    void createArcPatch();

    void updateHermiteCurve();

    void savePoints(Patch& patch);

    void loadProfile();

protected:
    int curveResolution{64};
    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } pTangentLines;

    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
        struct {
            std::array<int, 2> levels{1, 64};
        } constants;
    } pBezierCurve;

    VulkanBuffer bezierTangentIndices;
    VulkanBuffer hermiteTangentIndices;

    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
        struct {
            std::array<int, 2> levels{1, 64};
        } constants;
    } pHermiteCurve;



    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
        struct {
            std::array<int, 2> levels{1, 64};
            float angle{360};
            float radius{1};
            int clockwise{0};
        } constants;
    } pArcCurve;

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
            VulkanPipelineLayout layout;
            VulkanPipeline pipeline;
            bool enabled{true};
        } xray;
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
            float tessLevelOuter{50};
            float tessLevelInner{50};
            float u{1};
            float v{1};
            float radius{1};
            float height{3};
        } constants;
    } pCylinder;

    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
        struct{
            float tessLevelOuter{50};
            float tessLevelInner{50};
            float u{1};
            float v{1};
            float radius{0.5};
            float height{3};
        } constants;
    } pCone;

    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
        struct{
            VulkanPipelineLayout layout;
            VulkanPipeline pipeline;
            bool enabled{false};
        } xray;
        struct{
            float tessLevelOuter{50};
            float tessLevelInner{50};
            float u{1};
            float v{1};
            float r{0.5};
            float R{2};
        } constants;
    } pTorusSurface;

    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
        struct{
            float scale[3]{2, 2, 2};
            float tessLevelOuter{4};
            float tessLevelInner{4};
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
        struct{
            float tessLevelOuter{16};
            float tessLevelInner{16};
            float u{1};
            float v{1};
            int numPoints{0};
        } constants;
    } pSurfaceRevolution;

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
    Patch arc;
    Patch teapot;
    Patch teacup;
    Patch teaspoon;
    Patch sphere;
    Patch sweptSurface;
    Patch hermiteCurve;
    Patch icoSphere;
    Patch cube;
    Camera camera2d;
    std::unique_ptr<OrbitingCameraController> cameraController;
    int selectedPoint{-1};

    struct {
        glm::vec3 *handle = nullptr;
        Patch *source = nullptr;
        uint32_t numPoints{0};
    } point;
    std::vector<glm::vec3> pointsAdded;
    bool addingPoints{false};

    glm::vec3 mousePosition;
    bool pointSelected{false};
    Action* movePointAction = nullptr;
    Action* addPointAction = nullptr;
    float pointRadius = 0.1;
    static constexpr uint32_t maxPoints{6};

    std::array<VulkanBuffer, 10> indexBuffers;

    struct {
        VulkanBuffer buffer;
        VulkanDescriptorSetLayout layout;
        VkDescriptorSet descriptorSet;
    } mvp;

    struct {
        VulkanBuffer buffer;
        VulkanDescriptorSetLayout layout;
        VkDescriptorSet descriptorSet;
    } globalUBo;

    struct {
        Texture texture;
        VulkanDescriptorSetLayout layout;
        VkDescriptorSet descriptorSet;
    } vulkanImage;

    struct {
        VulkanDescriptorSetLayout layout;
        VkDescriptorSet descriptorSet;
        VulkanBuffer vertices;
        VulkanBuffer indices;
    } profileCurve;

    std::array<VkDescriptorSet, 3> descriptorSets{};

    static constexpr int MODE_CURVES = 0;
    static constexpr int MODE_SURFACE = 1;
    int mode{MODE_SURFACE};

    static constexpr int CURVE_BEZIER = 1 << 0;
    static constexpr int CURVE_HERMITE = 1 << 1;
    static constexpr int CURVE_ARC = 1 << 2;
    static constexpr int CURVES_WITH_CONTROL_POINTS = CURVE_HERMITE | CURVE_BEZIER;
    int curve{CURVE_HERMITE};

    static constexpr int SURFACE_TEAPOT = 1 << 0;
    static constexpr int SURFACE_TEACUP = 1 << 1;
    static constexpr int SURFACE_TEASPOON = 1 << 2;
    static constexpr int SURFACE_SPHERE = 1 << 3;
    static constexpr int SURFACE_ICO_SPHERE = 1 << 4;
    static constexpr int SURFACE_CUBE = 1 << 5;
    static constexpr int SURFACE_TORUS = 1 << 6;
    static constexpr int SURFACE_CYLINDER = 1 << 7;
    static constexpr int SURFACE_CONE = 1 << 8;
    static constexpr int SURFACE_SWEPT = 1 << 9;
    static constexpr int SURFACE_PLANE = 1 << 10;
    static constexpr int SURFACE_BEZIER = (SURFACE_TEAPOT | SURFACE_TEASPOON | SURFACE_TEACUP  | SURFACE_PLANE);
    static constexpr VkShaderStageFlags TESSELLATION_SHADER_STAGES_ALL = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    int surface{SURFACE_SWEPT};
    bool dirty{false};

    struct{
        alignas(16) glm::vec3 surfaceColor{1, 0, 0};
        struct {
            alignas(16) glm::vec3 color{1};
            float width{0.01};
            int enabled{0};
            int solid{1};
            int uvColor{0};
        } wireframe;
    } globalConstants;
    struct {
        bool active{false};
        ImVec2 pos{0, 0};
    } contextPopup;
    bool showTangentLines{true};
    bool showPoints{true};
};