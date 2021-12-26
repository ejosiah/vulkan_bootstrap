#pragma once
#include "Scene.hpp"


class AngularMomentumScene : public Scene {
public:
    AngularMomentumScene(VulkanDevice* device = nullptr, VulkanRenderPass* renderPass = nullptr, VulkanSwapChain* swapChain = nullptr
    , VulkanDescriptorPool* descriptorPool = nullptr, FileManager* fileManager = nullptr, Mouse* mouse = nullptr)
    : Scene(device, renderPass, swapChain, descriptorPool, fileManager, mouse)
    {}

    void init() override {
        createRenderPipeline();
        createSphere();
        createAxis();
    }

    void createSphere(){
        auto sphere = primitives::sphere(50, 50, 1.0f, glm::mat4{1}, glm::vec4(0), VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        auto vertexBuffer = m_device->createDeviceLocalBuffer(sphere.vertices.data(), BYTE_SIZE(sphere.vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        auto indexBuffer = m_device->createDeviceLocalBuffer(sphere.indices.data(), BYTE_SIZE(sphere.indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

        m_sphereEntity = createEntity("sphere");
        auto& renderComp = m_sphereEntity.add<component::Render>();
        renderComp.vertexBuffers.push_back(vertexBuffer);
        renderComp.indexBuffer = indexBuffer;
        renderComp.indexCount = sphere.indices.size();
        renderComp.instanceCount = 1;

        renderComp.primitives.push_back(vkn::Primitive::indexed(renderComp.indexCount, 0, sphere.vertices.size(), 0));

        auto& pipelines = m_sphereEntity.add<component::Pipelines>();
        component::Pipeline pipeline{ m_render.pipeline, m_render.layout };
        pipelines.add(pipeline);
    }

    void createAxis(){
        std::vector<glm::vec4> vertices{
            {0, 1.2, 0, 1}, {0, -1.2, 0, 1}, // tail
            {0, 1.2, 0, 1}, {-0.05, 1.150, 0, 1},  // left arrow
            {0, 1.2, 0, 1}, {0.05, 1.150, 0, 1}  // right arrow
        };
        auto vertexBuffer = m_device->createDeviceLocalBuffer(vertices.data(), BYTE_SIZE(vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

        m_axisEntity = createEntity("Axis");
        auto& renderComp = m_axisEntity.add<component::Render>();
        renderComp.vertexBuffers.push_back(vertexBuffer);
        renderComp.primitives.push_back(vkn::Primitive::Vertices(0, vertices.size()));

        auto& pipelines = m_axisEntity.add<component::Pipelines>();
        component::Pipeline pipeline{ m_axisRender.pipeline, m_axisRender.layout };
        pipelines.add(pipeline);
    }

    void update(float dt) override {
        if(m_flipAxis){
            axis *= -1;
            glm::quat flip = glm::angleAxis(glm::pi<float>(), right);
            auto& transform = m_axisEntity.get<component::Transform>();
            transform.value = glm::mat4(flip) * transform.value;
            right *= -1;
            m_flipAxis = false;
        }
        angularVelocity = glm::quat(0, speed * glm::normalize(axis));
        orientation = orientation + 0.5f * dt * angularVelocity * orientation;
        orientation = glm::normalize(orientation);
        m_sphereEntity.get<component::Transform>().value = glm::mat4(orientation);

    }

    void render(VkCommandBuffer commandBuffer) override {

    }

    void renderUI(VkCommandBuffer commandBuffer) override {
        ImGui::SliderFloat("speed", &speed, 0, 30);

        ImGui::Checkbox("Move Axis", &m_moveAxis); ImGui::SameLine();
        m_flipAxis |= ImGui::Button("Flip Axis");

    }

    void checkInputs() override {
        if (m_moveAxis) {
            float pitch = -m_mouse->relativePosition.y * 0.01f;
            float yaw = m_mouse->relativePosition.x * 0.01f;
            if (pitch != 0 || yaw != 0) {
                auto rotation = glm::mat3(glm::quat({pitch, yaw, 0.0f}));
                axis = rotation * axis;
                right = rotation * right;
                auto &transform = m_axisEntity.get<component::Transform>();
                transform.value = glm::mat4(rotation) * transform.value;
            }
        }
    }

    void createRenderPipeline() override {
        auto builder = m_device->graphicsPipelineBuilder();
        m_render.pipeline =
            builder
                .allowDerivatives()
                .shaderStage()
                  .vertexShader(load("torus.vert.spv"))
                    .fragmentShader(load("checkerboard.frag.spv"))
                .vertexInputState()
                    .addVertexBindingDescriptions(Vertex::bindingDisc())
                    .addVertexAttributeDescriptions(Vertex::attributeDisc())
                .inputAssemblyState()
                    .triangles()
                .viewportState()
                    .viewport()
                        .origin(0, 0)
                        .dimension(m_swapChain->extent)
                        .minDepth(0)
                        .maxDepth(1)
                    .scissor()
                        .offset(0, 0)
                        .extent(m_swapChain->extent)
                    .add()
                .rasterizationState()
                    .cullBackFace()
                    .frontFaceCounterClockwise()
                    .polygonModeFill()
                .depthStencilState()
                    .enableDepthWrite()
                    .enableDepthTest()
                    .compareOpLess()
                    .minDepthBounds(0)
                    .maxDepthBounds(1)
                .colorBlendState()
                    .attachment()
                    .add()
                .layout()
                  .addPushConstantRange(Camera::pushConstant())
                .renderPass(*m_renderPass)
                .subpass(0)
                .name("render")
            .build(m_render.layout);

        m_axisRender.pipeline =
            builder
                .setDerivatives()
                .basePipeline(m_render.pipeline)
                .shaderStage()
                    .vertexShader(load("axis.vert.spv"))
                    .fragmentShader(load("axis.frag.spv"))
                .vertexInputState()
                    .clear()
                    .addVertexBindingDescription(0, sizeof(glm::vec4), VK_VERTEX_INPUT_RATE_VERTEX)
                    .addVertexAttributeDescription(0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 0)
                .inputAssemblyState()
                    .lines()
                .rasterizationState()
                    .lineWidth(5.0)
                    .cullNone()
                .name("axis")
            .build(m_axisRender.layout);
    }


    void refresh() override {
        dispose(m_axisRender.pipeline);
        dispose(m_render.pipeline);

        createRenderPipeline();
        component::Pipeline pipeline{ m_render.pipeline, m_render.layout };
        auto pipelines = &m_sphereEntity.get<component::Pipelines>();
        pipelines->clear();
        pipelines->add(pipeline);

        pipeline = component::Pipeline{ m_axisRender.pipeline, m_axisRender.layout };
        pipelines = &m_axisEntity.get<component::Pipelines>();
        pipelines->clear();
        pipelines->add(pipeline);
    }

private:
    Entity m_sphereEntity;
    Entity m_axisEntity;
    glm::quat angularVelocity{0, 0, 0, 0};
    glm::vec3 axis{0, 1, 0};
    glm::vec3 right{1, 0, 0};
    float speed{5.0};
    bool m_moveAxis{false};
    bool m_flipAxis{false};
    glm::quat orientation{1, 0, 0, 0};
    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } m_axisRender;
};