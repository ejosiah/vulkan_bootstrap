#pragma once

#include "Scene.hpp"

struct RotationAxis{};
struct XAxis{};
struct YAxis{};
struct ZAxis{};

enum RotationOrder{
    XYZ, XZY, YXZ, YZX, ZXY, ZYX
};

class RotationHierarchyScene : public Scene{
public:
    RotationHierarchyScene(VulkanDevice* device = nullptr, VulkanRenderPass* renderPass = nullptr, VulkanSwapChain* swapChain = nullptr
            , VulkanDescriptorPool* descriptorPool = nullptr, FileManager* fileManager = nullptr, Mouse* mouse = nullptr)
    : Scene(device, renderPass, swapChain, descriptorPool, fileManager, mouse)
    {}

    void init() override {
        createRenderPipeline();
        loadModel();
        createRotationAxis();
        updateModelParentTransform();
    }

    void loadModel(){
        VulkanDrawable model;
        phong::load("../../data/models/bigship1.obj", *m_device, *m_descriptorPool, model);

        spaceShip = createEntity("model");
        auto& renderComp = spaceShip.add<component::Render>();
        renderComp.vertexBuffers.push_back(model.vertexBuffer);
        renderComp.indexBuffer = model.indexBuffer;
        renderComp.indexCount = model.numTriangles() * 3;
        renderComp.instanceCount = 1;

        for(auto& mesh : model.meshes){
            renderComp.primitives.push_back(mesh);
        }

        auto& pipelines = spaceShip.add<component::Pipelines>();
        component::Pipeline pipeline{ m_render.pipeline, m_render.layout };
        pipelines.add(pipeline);
    }

    void update(float dt) override {
        if(orderUpdated){
            auto view = m_registry.view<RotationAxis>();
            std::vector<entt::entity> entities;
            for(auto entity : view){
                entities.push_back(entity);
            }
            m_registry.destroy(begin(entities), end(entities));
            createRotationAxis();
            updateModelParentTransform();
            axisRotation = glm::vec3(0);
        }

        if(glm::any(glm::notEqual(axisRotation, glm::vec3(0)))){
            auto xView = m_registry.view<XAxis, component::Transform>();
            for(auto entity : xView){
                xView.get<component::Transform>(entity).value = glm::rotate(glm::mat4(1), glm::radians(axisRotation.x), {1, 0, 0});
            }
            auto yView = m_registry.view<YAxis, component::Transform>();
            for(auto entity : yView){
                yView.get<component::Transform>(entity).value = glm::rotate(glm::mat4(1), glm::radians(axisRotation.y), {0, 1, 0});
            }
            auto zView = m_registry.view<ZAxis, component::Transform>();
            for(auto entity : zView){
                zView.get<component::Transform>(entity).value = glm::rotate(glm::mat4(1), glm::radians(axisRotation.z), {0, 0, 1});
            }

            auto view = m_registry.view<RotationAxis, component::Transform>();
            for(auto entity : view){
                auto& transform = view.get<component::Transform>(entity);
                if(transform.parent){
                    transform.value = transform.parent->value * transform.value;
                }
            }

            auto& transform = spaceShip.get<component::Transform>();
            transform.value = transform.parent->value;
        }
    }

    void render(VkCommandBuffer commandBuffer) override {

    }

    void renderUI(VkCommandBuffer commandBuffer) override {
        auto previousOrder = rotationOrder;
        ImGui::Text("Rotation Order");
        ImGui::RadioButton("XYZ", &rotationOrder, XYZ); ImGui::SameLine();
        ImGui::RadioButton("XZY", &rotationOrder, XZY); ImGui::SameLine();
        ImGui::RadioButton("YXZ", &rotationOrder, YXZ);

        ImGui::RadioButton("YZX", &rotationOrder, YZX); ImGui::SameLine();
        ImGui::RadioButton("ZXY", &rotationOrder, ZXY); ImGui::SameLine();
        ImGui::RadioButton("ZYX", &rotationOrder, ZYX);
        orderUpdated = previousOrder != rotationOrder;


        float range = 90.0f;
        auto renderOrder = [](const char* aLabel, const char* bLabel, const char* cLabel, float* a, float* b, float* c, float range){
            if(ImGui::TreeNodeEx(aLabel, ImGuiTreeNodeFlags_DefaultOpen)){
                ImGui::SliderFloat(aLabel, a, -range, range);

                ImGui::SetNextItemOpen(true, ImGuiCond_Once);
                if(ImGui::TreeNode(bLabel)){
                    ImGui::SliderFloat(bLabel, b, -range, range);

                    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
                    if(ImGui::TreeNode(cLabel)) {
                        ImGui::SliderFloat(cLabel, c, -range, range);
                        ImGui::TreePop();
                    }
                    ImGui::TreePop();
                }
                ImGui::TreePop();
            }
        };

        ImGui::Text("Rotations");
        if(rotationOrder == XYZ){
            renderOrder("X", "Y", "Z", &axisRotation.x, &axisRotation.y, &axisRotation.z, range);
        }else if(rotationOrder == XZY){
            renderOrder("X", "Z", "Y", &axisRotation.x, &axisRotation.z, &axisRotation.y, range);
        }else if(rotationOrder == YXZ){
            renderOrder("Y", "X", "Z", &axisRotation.y, &axisRotation.x, &axisRotation.z, range);
        }else if(rotationOrder == YZX){
            renderOrder("Y", "Z", "X", &axisRotation.y, &axisRotation.z, &axisRotation.x, range);
        }else if(rotationOrder == ZXY){
            renderOrder("Z", "X", "Y", &axisRotation.z, &axisRotation.x, &axisRotation.y, range);
        }else if(rotationOrder == ZYX){
            renderOrder("Z", "Y", "X", &axisRotation.z, &axisRotation.y, &axisRotation.x, range);
        }
    }

    void createRenderPipeline() override{
        auto builder = m_device->graphicsPipelineBuilder();
        m_render.pipeline =
            builder
                .shaderStage()
                  .vertexShader(load("torus.vert.spv"))
                    .fragmentShader(load("torus.frag.spv"))
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
    }

    void refresh() override {
        createRenderPipeline();
        auto view = m_registry.view<component::Pipelines>();
        component::Pipeline pipeline{ m_render.pipeline, m_render.layout };
        for(auto entity : view){
            auto& pipelines = view.get<component::Pipelines>(entity);
            pipelines.clear();
            pipelines.add(pipeline);
        }
    }

    void updateModelParentTransform(){
        component::Transform* parent = nullptr;
        auto tView = m_registry.view<RotationAxis, component::Transform>();
        for(auto entity : tView){
            parent = &tView.get<component::Transform>(entity);  // last oldest entry is parent
        };
        spaceShip.get<component::Transform>().parent = parent;
    }

    void createRotationAxis(){
        if(rotationOrder == XYZ) {
            auto zAxis = createAxis("z_axis", 1, 0.05, {0, 0, 1}, {1, 0, 0, 0});
            zAxis.add<ZAxis>();

            auto orient = glm::angleAxis(-glm::half_pi<float>(), glm::vec3{1, 0, 0});
            auto yAxis = createAxis("y_axis", 1.10, 0.05, {0, 1, 0}, orient);
            yAxis.add<YAxis>();

            orient = glm::angleAxis(-glm::half_pi<float>(), glm::vec3{0, 1, 0});
            auto xAxis = createAxis("x_axis", 1.20, 0.05, {1, 0, 0}, orient);
            xAxis.add<XAxis>();

            zAxis.get<component::Transform>().parent = &yAxis.get<component::Transform>();
            yAxis.get<component::Transform>().parent = &xAxis.get<component::Transform>();
        }else if(rotationOrder == XZY){
            auto orient = glm::angleAxis(-glm::half_pi<float>(), glm::vec3{1, 0, 0});
            auto yAxis = createAxis("y_axis", 1, 0.05, {0, 1, 0}, orient);
            yAxis.add<YAxis>();

            auto zAxis = createAxis("z_axis", 1.10, 0.05, {0, 0, 1}, {1, 0, 0, 0});
            zAxis.add<ZAxis>();

            orient = glm::angleAxis(-glm::half_pi<float>(), glm::vec3{0, 1, 0});
            auto xAxis = createAxis("x_axis", 1.20, 0.05, {1, 0, 0}, orient);
            xAxis.add<XAxis>();

            yAxis.get<component::Transform>().parent = &zAxis.get<component::Transform>();
            zAxis.get<component::Transform>().parent = &xAxis.get<component::Transform>();
        }else if (rotationOrder == YXZ){
            auto zAxis = createAxis("z_axis", 1, 0.05, {0, 0, 1}, {1, 0, 0, 0});
            zAxis.add<ZAxis>();

            auto orient = glm::angleAxis(-glm::half_pi<float>(), glm::vec3{0, 1, 0});
            auto xAxis = createAxis("x_axis", 1.10, 0.05, {1, 0, 0}, orient);
            xAxis.add<XAxis>();

            orient = glm::angleAxis(-glm::half_pi<float>(), glm::vec3{1, 0, 0});
            auto yAxis = createAxis("y_axis", 1.20, 0.05, {0, 1, 0}, orient);
            yAxis.add<YAxis>();

            zAxis.get<component::Transform>().parent = &xAxis.get<component::Transform>();
            xAxis.get<component::Transform>().parent = &yAxis.get<component::Transform>();
        }else if(rotationOrder == YZX){
            auto orient = glm::angleAxis(-glm::half_pi<float>(), glm::vec3{0, 1, 0});
            auto xAxis = createAxis("x_axis", 1, 0.05, {1, 0, 0}, orient);
            xAxis.add<XAxis>();

            auto zAxis = createAxis("z_axis", 1.10, 0.05, {0, 0, 1}, {1, 0, 0, 0});
            zAxis.add<ZAxis>();

            orient = glm::angleAxis(-glm::half_pi<float>(), glm::vec3{1, 0, 0});
            auto yAxis = createAxis("y_axis", 1.20, 0.05, {0, 1, 0}, orient);
            yAxis.add<YAxis>();

            xAxis.get<component::Transform>().parent = &zAxis.get<component::Transform>();
            zAxis.get<component::Transform>().parent = &yAxis.get<component::Transform>();
        }else if(rotationOrder == ZXY){
            auto orient = glm::angleAxis(-glm::half_pi<float>(), glm::vec3{1, 0, 0});
            auto yAxis = createAxis("y_axis", 1, 0.05, {0, 1, 0}, orient);
            yAxis.add<YAxis>();

            orient = glm::angleAxis(-glm::half_pi<float>(), glm::vec3{0, 1, 0});
            auto xAxis = createAxis("x_axis", 1.10, 0.05, {1, 0, 0}, orient);
            xAxis.add<XAxis>();

            auto zAxis = createAxis("z_axis", 1.20, 0.05, {0, 0, 1}, {1, 0, 0, 0});
            zAxis.add<ZAxis>();

            yAxis.get<component::Transform>().parent = &xAxis.get<component::Transform>();
            xAxis.get<component::Transform>().parent = &zAxis.get<component::Transform>();
        }else if(rotationOrder == ZYX){
            auto orient = glm::angleAxis(-glm::half_pi<float>(), glm::vec3{0, 1, 0});
            auto xAxis = createAxis("x_axis", 1, 0.05, {1, 0, 0}, orient);
            xAxis.add<XAxis>();

            orient = glm::angleAxis(-glm::half_pi<float>(), glm::vec3{1, 0, 0});
            auto yAxis = createAxis("y_axis", 1.10, 0.05, {0, 1, 0}, orient);
            yAxis.add<YAxis>();

            auto zAxis = createAxis("z_axis", 1.20, 0.05, {0, 0, 1}, {1, 0, 0, 0});
            zAxis.add<ZAxis>();

            xAxis.get<component::Transform>().parent = &yAxis.get<component::Transform>();
            yAxis.get<component::Transform>().parent = &zAxis.get<component::Transform>();
        }
    }

    Entity createAxis(const std::string& name, float innerR, float outerR, glm::vec3 color, glm::quat orientation){
        auto torus = primitives::torus(50, 50, innerR, outerR, glm::mat4(orientation), glm::vec4(color, 1), VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

        auto vertices = m_device->createDeviceLocalBuffer(torus.vertices.data(), BYTE_SIZE(torus.vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        auto indices = m_device->createDeviceLocalBuffer(torus.indices.data(), BYTE_SIZE(torus.indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

        auto entity = createEntity(name);
        entity.add<RotationAxis>();

        auto& renderComp = entity.add<component::Render>();
        renderComp.vertexBuffers.push_back(vertices);
        renderComp.indexBuffer = indices;
        uint32_t indexCount = torus.indices.size();
        uint32_t vertexCount = torus.vertices.size();
        renderComp.primitives.push_back(vkn::Primitive::indexed(indexCount, 0, vertexCount, 0));
        renderComp.instanceCount = 1;
        renderComp.indexCount = indexCount;

        auto& pipelines = entity.add<component::Pipelines>();
        component::Pipeline pipeline{ m_render.pipeline, m_render.layout };
        pipelines.add(pipeline);

        return entity;
    }


private:
    glm::vec3 axisRotation{0};
    int rotationOrder = XYZ;
    bool orderUpdated = false;
    Entity spaceShip;
};