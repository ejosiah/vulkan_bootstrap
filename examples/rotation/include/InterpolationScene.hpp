#pragma once

#include "Scene.hpp"

class InterpolationScene : public Scene{
public:
    InterpolationScene(VulkanDevice* device = nullptr, VulkanRenderPass* renderPass = nullptr, VulkanSwapChain* swapChain = nullptr
    , VulkanDescriptorPool* descriptorPool = nullptr, FileManager* fileManager = nullptr, Mouse* mouse = nullptr)
    : Scene(device, renderPass, swapChain, descriptorPool, fileManager, mouse)
    {}
    
    void init() final {
        createRenderPipeline();
        createSphere();
        createAxis(m_axisAEntity, {1, 0, 0, 1});
        createAxis(m_axisBEntity, {0, 1, 0, 1});
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

    void createAxis(Entity& entity, glm::vec4 color){
        std::vector<Vertex> vertices{
                {{0, 1.2, 0, 1}, color}, {{0, -1.2, 0, 1}, color}, // tail
                {{0, 1.2, 0, 1}, color}, {{-0.05, 1.150, 0, 1}, color},  // left arrow
                {{0, 1.2, 0, 1}, color}, {{0.05, 1.150, 0, 1}, color}  // right arrow
        };
        auto vertexBuffer = m_device->createDeviceLocalBuffer(vertices.data(), BYTE_SIZE(vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

        entity = createEntity("Axis");
        auto& renderComp = entity.add<component::Render>();
        renderComp.vertexBuffers.push_back(vertexBuffer);
        renderComp.primitives.push_back(vkn::Primitive::Vertices(0, vertices.size()));

        auto& pipelines = entity.add<component::Pipelines>();
        component::Pipeline pipeline{ m_axisRender.pipeline, m_axisRender.layout };
        pipelines.add(pipeline);
    }

    void update(float dt) final {
        if(m_doLerp){
            float t = glm::clamp(0.f, 1.f, m_elapsed/m_duration);

            auto quatA = glm::quat(m_axisAEntity.get<component::Transform>().value);
            auto quatB = glm::quat(m_axisBEntity.get<component::Transform>().value);
            auto quatC = m_lerpMethod == 0 ? glm::lerp(quatA, quatB, t) : glm::slerp(quatA, quatB, t);

            m_sphereEntity.get<component::Transform>().value = glm::mat4(quatC);
            m_elapsed += dt;

            if(m_elapsed >= m_duration){
                m_doLerp = false;
                m_elapsed = 0.0f;
            }
        }
    }

    void render(VkCommandBuffer commandBuffer) final {

    }

    void renderUI(VkCommandBuffer commandBuffer) final {
        ImGui::SliderFloat("duration", &m_duration, 0.5, 5);
        ImGui::Text("Axis");
        ImGui::Checkbox("Move Axis", &m_moveAxis);
        if(m_moveAxis){
            ImGui::SameLine();
            ImGui::RadioButton("Axis A", &m_axisId, 0);
            ImGui::SameLine();
            ImGui::RadioButton("Axis B", &m_axisId, 1);
        }
        ImGui::Text("Interpolation");
        ImGui::RadioButton("lerp", &m_lerpMethod, 0); ImGui::SameLine();
        ImGui::RadioButton("slerp", &m_lerpMethod, 1);
        m_doLerp |= ImGui::Button("Interpolate");
    }

    void createRenderPipeline() final {
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
                    .addVertexBindingDescription(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX)
                    .addVertexAttributeDescription(0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetOf(Vertex, position))
                    .addVertexAttributeDescription(1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetOf(Vertex, color))
                .inputAssemblyState()
                    .lines()
                .rasterizationState()
                    .lineWidth(5.0)
                    .cullNone()
                .name("axis")
            .build(m_axisRender.layout);
    }

    void checkInputs() final {
        if (m_moveAxis) {
            float pitch = -m_mouse->relativePosition.y * 0.01f;
            float yaw = m_mouse->relativePosition.x * 0.01f;
            if (pitch != 0 || yaw != 0) {
                auto& axis = m_axisId == 0 ? m_axisA : m_axisB;
                auto rotation = glm::mat3(glm::quat({pitch, yaw, 0.0f}));
                axis = rotation * axis;
                auto entity = m_axisId == 0 ? m_axisAEntity : m_axisBEntity;
                auto &transform = entity.get<component::Transform>();
                transform.value = glm::mat4(rotation) * transform.value;
            }
        }
    }

    void refresh() final {
        dispose(m_axisRender.pipeline);
        dispose(m_render.pipeline);

        createRenderPipeline();
        component::Pipeline pipeline{ m_render.pipeline, m_render.layout };
        auto pipelines = &m_sphereEntity.get<component::Pipelines>();
        pipelines->clear();
        pipelines->add(pipeline);

        pipeline = component::Pipeline{ m_axisRender.pipeline, m_axisRender.layout };
        pipelines = &m_axisAEntity.get<component::Pipelines>();
        pipelines->clear();
        pipelines->add(pipeline);

        pipeline = component::Pipeline{ m_axisRender.pipeline, m_axisRender.layout };
        pipelines = &m_axisBEntity.get<component::Pipelines>();
        pipelines->clear();
        pipelines->add(pipeline);
    }

private:
    Entity m_sphereEntity;
    Entity m_axisAEntity;
    Entity m_axisBEntity;
    float m_duration{2};
    float m_elapsed{0};
    bool m_moveAxis{false};
    bool m_doLerp{false};
    int m_axisId{0};
    int m_lerpMethod{0};
    glm::vec3 m_axisA{0, 1, 0};
    glm::vec3 m_axisB{0, 1, 0};
    struct {
        VulkanPipelineLayout layout;
        VulkanPipeline pipeline;
    } m_axisRender;
};