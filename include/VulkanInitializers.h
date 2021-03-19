#pragma once

#include <vulkan/vulkan.h>

namespace initializers{

    struct ShaderInfo{
        VkShaderModule module;
        VkShaderStageFlagBits stage;
        const char*  entry = "main";
    };

    static inline std::vector<VkPipelineShaderStageCreateInfo> vertexShaderStages(const std::vector<ShaderInfo>& shaderInfos){
        std::vector<VkPipelineShaderStageCreateInfo> createInfos;

        for(auto& shaderInfo : shaderInfos){
            VkPipelineShaderStageCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            createInfo.stage = shaderInfo.stage;
            createInfo.module = shaderInfo.module;
            createInfo.pName = shaderInfo.entry;

            createInfos.push_back(createInfo);
        }

        return createInfos;
    }

    inline VkPipelineVertexInputStateCreateInfo vertexInputState(const std::vector<VkVertexInputBindingDescription>& bindings = {}, const std::vector<VkVertexInputAttributeDescription>& attributes = {}){
        VkPipelineVertexInputStateCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        createInfo.vertexBindingDescriptionCount = COUNT(bindings);
        createInfo.pVertexBindingDescriptions = bindings.data();
        createInfo.vertexAttributeDescriptionCount = COUNT(attributes);
        createInfo.pVertexAttributeDescriptions = attributes.data();

        return createInfo;
    }

    static inline VkPipelineInputAssemblyStateCreateInfo inputAssemblyState(VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VkBool32 primitiveRestart = VK_FALSE){
        VkPipelineInputAssemblyStateCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        createInfo.topology = topology;
        createInfo.primitiveRestartEnable = primitiveRestart;

        return createInfo;
    }

    inline VkViewport viewport(float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f){
        VkViewport viewport{};
        viewport.width = width;
        viewport.height = height;
        viewport.minDepth = minDepth;
        viewport.maxDepth = maxDepth;

        return viewport;
    }

    inline VkViewport viewport(const VkExtent2D& extent, float minDepth = 0.0f, float maxDepth = 1.0f){
        VkViewport viewport{};
        viewport.width = static_cast<float>(extent.width);
        viewport.height = static_cast<float>(extent.height);
        viewport.minDepth = minDepth;
        viewport.maxDepth = maxDepth;

        return viewport;
    }

    inline VkRect2D scissor(uint32_t width, uint32_t height, int32_t offsetX = 0, int32_t offsetY = 0){
        VkRect2D rect{};
        rect.extent.width = width;
        rect.extent.height = height;
        rect.offset.x = offsetX;
        rect.offset.y = offsetY;

        return rect;

    }

    inline VkPipelineViewportStateCreateInfo viewportState(const std::vector<VkViewport>& viewports, const std::vector<VkRect2D>& scissors){
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = COUNT(viewports);
        viewportState.pViewports = viewports.data();
        viewportState.scissorCount = COUNT(scissors);
        viewportState.pScissors = scissors.data();

        return viewportState;
    }

    inline VkPipelineViewportStateCreateInfo viewportState(const VkViewport& viewport, const VkRect2D& scissor){
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        return viewportState;
    }

    inline VkPipelineRasterizationStateCreateInfo rasterizationState(){
        VkPipelineRasterizationStateCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

        return createInfo;
    }

    inline VkPipelineMultisampleStateCreateInfo multisampleState(){
        VkPipelineMultisampleStateCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        createInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        return createInfo;
    }

    inline VkPipelineDepthStencilStateCreateInfo depthStencilState(){
        VkPipelineDepthStencilStateCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        createInfo.depthTestEnable = VK_FALSE;
        createInfo.depthWriteEnable = VK_FALSE;

        return createInfo;
    }

    static inline VkPipelineColorBlendStateCreateInfo colorBlendState(VkPipelineColorBlendAttachmentState blendAttachmentState = {}){
        if(blendAttachmentState.colorWriteMask == 0){
            blendAttachmentState.colorWriteMask =
                    VK_COLOR_COMPONENT_R_BIT
                    | VK_COLOR_COMPONENT_G_BIT
                    | VK_COLOR_COMPONENT_B_BIT
                    | VK_COLOR_COMPONENT_A_BIT;
        }
        VkPipelineColorBlendStateCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        createInfo.attachmentCount = 1;
        createInfo.pAttachments = &blendAttachmentState;

        return createInfo;
    }

    inline VkPipelineDynamicStateCreateInfo dynamicState(std::vector<VkDynamicState> dynamicStates  = {}) {
        VkPipelineDynamicStateCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        createInfo.dynamicStateCount = COUNT(dynamicStates);
        createInfo.pDynamicStates = dynamicStates.data();

        return createInfo;
    }

    inline VkRenderPassCreateInfo renderPassCreateInfo(
              const std::vector<VkAttachmentDescription>& attachmentDescriptions
            , const std::vector<VkSubpassDescription>& subpassDescriptions
            , const std::vector<VkSubpassDependency>& dependencies = {}){

        VkRenderPassCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        createInfo.attachmentCount = COUNT(attachmentDescriptions);
        createInfo.pAttachments = attachmentDescriptions.data();
        createInfo.subpassCount = COUNT(subpassDescriptions);
        createInfo.pSubpasses = subpassDescriptions.data();
        createInfo.dependencyCount = COUNT(dependencies);
        createInfo.pDependencies = dependencies.data();

        return createInfo;
    }

    inline VkFramebufferCreateInfo framebufferCreateInfo(
            VkRenderPass renderPass
            , const std::vector<VkImageView>& attachments
            , uint32_t width
            , uint32_t height
            , uint32_t layers = 1){

        VkFramebufferCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        createInfo.renderPass = renderPass;
        createInfo.attachmentCount = COUNT(attachments);
        createInfo.pAttachments = attachments.data();
        createInfo.width = width;
        createInfo.height = height;
        createInfo.layers = layers;

        return createInfo;
    }
}