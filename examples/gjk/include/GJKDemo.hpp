#include "VulkanBaseApp.h"
#include "CSOView.hpp"
#include "imgui.h"

struct SphereBody{
    glm::vec3 position{0};
    float radius{0.5};
    glm::mat3 orientation{1};
    VulkanBuffer vertices;
    VulkanBuffer indices;

    [[nodiscard]]
    glm::vec3 support(const glm::vec3& direction, const glm::vec3& origin
                      , const glm::mat3& orient, float bias = 0) const {
        return  origin + direction * (radius + bias);
    }
};

struct BoxBody{
    std::array<glm::vec3, 8> corners;
    glm::mat3 orientation{1};
    glm::vec3 position{0};
    VulkanBuffer vertices;
    VulkanBuffer indices;

    [[nodiscard]]
    glm::mat4 model() const {
        glm::mat4 xform = glm::translate(glm::mat4{1}, position);
        return xform * glm::mat4(orientation);
    }

    void buildCorners(){
        glm::vec3 max{-1000};
        glm::vec3 min{1000};

        auto numPoints = vertices.sizeAs<Vertex>();
        auto points = reinterpret_cast<Vertex*>(vertices.map());
        for(int i = 0; i < numPoints; i++){
            max = glm::max(max, points[i].position.xyz());
            min = glm::min(min, points[i].position.xyz());
        }

        corners[0] = glm::vec3{min.x, min.y, min.z};
        corners[1] = glm::vec3{min.x, min.y, max.z};
        corners[2] = glm::vec3{min.x, max.y, max.z};
        corners[3] = glm::vec3{min.x, max.y, min.z};

        corners[4] = glm::vec3{max.x, max.y, max.z};
        corners[5] = glm::vec3{max.x, max.y, min.z};
        corners[6] = glm::vec3{max.x, min.y, min.z};
        corners[7] = glm::vec3{max.x, min.y, max.z};

    }

    [[nodiscard]]
    glm::vec3 support(const glm::vec3& direction, const glm::vec3& origin
                      , const glm::mat3& orient, float bias = 0) const {
        auto maxPoint = orient * corners[0] + origin;
        auto maxDist = glm::dot(direction, maxPoint);
        for(auto i = 1; i < corners.size(); i++){
            const auto pt = orient * corners[i] + origin;
            const auto dist = glm::dot(direction, pt);
            if(dist > maxDist){
                maxDist = dist;
                maxPoint = pt;
            }
        }
        auto n = glm::normalize(direction);
        n *= bias;
        return maxPoint + n;
    }
};

class GJKDemo : public VulkanBaseApp{
public:
    explicit GJKDemo(const Settings& settings = {});

protected:
    void initApp() override;

    void initCSOView();

    void createDescriptorPool();

    void createDescriptorSetLayouts();

    void updateDescriptorSets();

    void createCommandPool();

    void createPipelineCache();

    void createRenderPipeline();

    void createComputePipeline();

    void onSwapChainDispose() override;

    void onSwapChainRecreation() override;

    VkCommandBuffer *buildCommandBuffers(uint32_t imageIndex, uint32_t &numCommandBuffers) override;

    void renderCSO(VkCommandBuffer commandBuffer);

    void update(float time) override;

    void newFrame() override;

    void checkAppInputs() override;

    void cleanup() override;

    void onPause() override;

    void createShapes();

    void initCameras();

    void collisionTest();

protected:
    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } render;

    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } compute;

    VulkanDescriptorPool descriptorPool;
    VulkanCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    VulkanPipelineCache pipelineCache;
    SphereBody sphere;
    BoxBody box;
    CSOView csoView;
    Texture vulkanImage;
    ImTextureID vulkanImageTexId{nullptr};
    ImTextureID csoTexId{ nullptr };
    VkDescriptorSet vulkanImageDescriptorSet;
    VulkanDescriptorSetLayout textureDescriptorSetLayout;
    std::unique_ptr<OrbitingCameraController> cameraController;
    struct {
        Action* forward{nullptr};
        Action* back{nullptr};
        Action* left{nullptr};
        Action* right{nullptr};
        Action* up{nullptr};
        Action* down{nullptr};
    } move;

    bool csoCam = false;
};