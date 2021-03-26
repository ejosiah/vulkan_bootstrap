#include <glm/gtc/type_ptr.hpp>
#include <VulkanShaderModule.h>
#include "VulkanInitializers.h"
#include "Font.h"
#include "xforms.h"


Font::Font(std::string_view name, uint32_t swapChainImageCount, uint32_t* index, int size, FontStyle style, const glm::vec3& color)
: size(size)
, style(style)
, color(color)
, texts(swapChainImageCount)
, currentImageIndex(index)
{
    auto font = Fonts::getFontName(name, style);
    const char* path = Fonts::location[font].c_str();

    FT_Error error = FT_New_Face(Fonts::ftLibrary, path, 0, &face);
    if(error){
        auto msg = "Error loading font:" + font + ", reason:" +  std::string(FT_Error_String(error));
        spdlog::error(msg);
        throw std::runtime_error{ msg };
    }
    FT_Set_Pixel_Sizes(face, 0, size);
    load();
}

void Font::load() {
    for(unsigned char chr = 0; chr < NUM_CHAR; chr++){
        auto error = FT_Load_Char(face, chr, FT_LOAD_RENDER);
        if(error){
            auto msg = std::string{"Unable to load char:"} + static_cast<char>(chr) + ", reason: " + FT_Error_String(error);
            spdlog::warn(msg);
            continue;
        }
        auto glyph = face->glyph;
        if(glyph->bitmap.width * glyph->bitmap.rows == 0) continue;
        auto& character = characters[chr];
        Fonts::createTexture(face, character.texture);
        character.value = static_cast<char>(chr);
        character.size = { glyph->bitmap.width, face->glyph->bitmap.rows };
        character.bearing = { glyph->bitmap_left, face->glyph->bitmap_top };
        character.advance = glyph->advance.x;
        character.loaded = true;

        maxWidth = std::max(maxWidth, glyph->bitmap.width);
        maxHeight = std::max(maxHeight, glyph->bitmap.rows);
    }
    createColorBuffer();
    createDescriptorSets();

}

void Font::createColorBuffer() {
    auto& device = *Fonts::device;
    VkDeviceSize size = sizeof(glm::vec3);

    VulkanBuffer stagingBuffer = device.createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, size, "Staging buffer");
    stagingBuffer.copy(&color, size);

    colorBuffer = device.createBuffer(
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY,
            size, "Color Buffer");

    Fonts::commandPool.oneTimeCommand(device.queues.graphics, [&](VkCommandBuffer cmdBuffer){
        VkBufferCopy copy{};
        copy.size = size;
        copy.dstOffset = 0;
        copy.srcOffset = 0;
        vkCmdCopyBuffer(cmdBuffer, stagingBuffer, colorBuffer, 1u, &copy);
    });
}

void Font::createDescriptorSets() {
    auto descriptorSets = Fonts::createDescriptorSets();
    for(int i = 0; i < NUM_CHAR; i++){
        auto& character = characters[i];
        if(!character.loaded) continue;
        character.descriptorSet = descriptorSets[i];


        std::array<VkDescriptorBufferInfo, 2> bufferInfo{};
        bufferInfo[0].buffer = Fonts::projectionBuffer;
        bufferInfo[0].offset = 0;
        bufferInfo[0].range = sizeof(glm::mat4);


        bufferInfo[1].buffer = colorBuffer;
        bufferInfo[1].offset = 0;
        bufferInfo[1].range = sizeof(glm::vec3);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.sampler = character.texture.sampler;
        imageInfo.imageView = character.texture.imageView;
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        std::array<VkWriteDescriptorSet, 3> writes{};
        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = character.descriptorSet;
        writes[0].dstBinding = 0;
        writes[0].dstArrayElement = 0;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[0].pBufferInfo = &bufferInfo[0];
        writes[0].descriptorCount = 1;

        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstSet = character.descriptorSet;
        writes[1].dstBinding = 1;
        writes[1].dstArrayElement = 0;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[1].pImageInfo = &imageInfo;
        writes[1].descriptorCount = 1;

        writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[2].dstSet = character.descriptorSet;
        writes[2].dstBinding = 2;
        writes[2].dstArrayElement = 0;
        writes[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[2].pBufferInfo = &bufferInfo[1];
        writes[2].descriptorCount = 1;


        vkUpdateDescriptorSets(*Fonts::device, COUNT(writes), writes.data(), 0, VK_NULL_HANDLE);
    }
}


Font::Font(Font &&source) noexcept {
    operator=(static_cast<Font&&>(source));
}

Font &Font::operator=(Font &&source) noexcept {
    if(&source == this) return *this;
    this->size = source.size;
    this->style = source.style;
    this->color = source.color;
    this->face = source.face;
    this->characters = std::move(source.characters);
    this->texts = std::move(source.texts);
    this->colorBuffer = std::move(source.colorBuffer);
    this->maxWidth = source.maxWidth;
    this->maxHeight = source.maxHeight;
    this->currentImageIndex = source.currentImageIndex;

    source.face = nullptr;
    source.currentImageIndex = nullptr;

    return *this;
}


glm::ivec2 Font::sizeOf(const std::string& text) const {
    return glm::ivec2();
}


void Font::write(const std::string &text, float x, float y, float d) {
    y = Fonts::screenHeight - y;
    float startX = x;
    glm::vec4 box[4];
    for(auto c : text){
        if(c == '\n'){
            y += (maxHeight + 2.0f) * d;
            x = startX;
            continue;
        }
        if(c == '\t'){
            x += maxWidth * 2.0f;
            continue;
        }
        if(c == ' '){
            x += maxWidth * Fonts::worldSpacing;
            continue;
        }
        auto& ch = characters[c];

        float x2 = x + ch.bearing.x;
        float y2 = y - (static_cast<float>(ch.size.y) - ch.bearing.y);
        float w = ch.size.x;
        float h = ch.size.y;

        box[0] = { x2, y2 + h    , 0, 0 };
        box[1] = { x2, y2, 0, 1 };
        box[2] = { x2 + w, y2 + h, 1, 0 };
        box[3] = { x2 + w, y2    , 1, 1 };

        x += (ch.advance >> 6u);
        CharacterInstance chInst;
        chInst.character = &ch;
        chInst.buffer = Fonts::createVertexBuffer();
        chInst.buffer.copy(box, sizeof(glm::vec4) * NUM_VERTICES);
        texts[*currentImageIndex].push_back((std::move(chInst)));
    }
}

void Font::clear() {
    texts[*currentImageIndex].clear();
}

void Font::draw(VkCommandBuffer commandBuffer) const {
    VkDeviceSize offset = 0;
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Fonts::pipeline);
    for(auto& ch : texts[*currentImageIndex]){
        vkCmdBindDescriptorSets(
                commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Fonts::pipelineLayout,
                0u, 1u, &ch.character->descriptorSet, 0, VK_NULL_HANDLE);
        vkCmdBindVertexBuffers(commandBuffer, 0u, 1u, &ch.buffer.buffer, &offset);
        vkCmdDraw(commandBuffer, 4u, 1u, 0u, 0u);
    }
}

void Font::refresh() {
    createDescriptorSets();
}



// ===================================== Fonts =================================

Font* Fonts::getFont(std::string_view name, int size, FontStyle style, const glm::vec3 &color) {
    auto colorStr = std::to_string(color.r) + std::to_string(color.g) + std::to_string(color.b);
    auto key = getFontName(name, style) + std::to_string(size) + colorStr;
    auto itr = fonts.find(key);
    if(itr == end(fonts)){
        auto font = Font{name, swapChainImageCount, currentImageIndex, size, style, color};
        fonts.insert(std::make_pair(key, std::move(font)));
    }
    return &fonts[key];
}

void Fonts::init(VulkanDevice* dev, VulkanRenderPass* rPass, uint32_t rSubpass, uint32_t swapChainImgCount, uint32_t* index, int width, int height) {
    FT_Error error;
    if((error = FT_Init_FreeType(&ftLibrary))){
        auto msg = "Error loading FT font, reason:" +  std::string(FT_Error_String(error));
        spdlog::error(msg);
        throw std::runtime_error{ msg };
    }
    screenWidth = static_cast<float>(width);
    screenHeight = static_cast<float>(height);
    device = dev;
    renderPass = rPass;
    subpass = rSubpass;
    swapChainImageCount = swapChainImgCount;
    currentImageIndex = index;

#ifdef _WIN32
    location[COURIER] = R"(C:\Windows\Fonts\cour.ttf)";
    location[ARIAL] = R"(C:\Windows\Fonts\arial.ttf)";
    location[ARIAL_BOLD] = R"(C:\Windows\Fonts\arialbd.ttf)";
    location[ARIAL_BOLD_ITALIC] = R"(C:\Windows\Fonts\arialbi.ttf)";
    location[ARIAL_ITALIC] = R"(C:\Windows\Fonts\ariali.ttf)";
    location[JET_BRAINS_MONO] = R"(C:\Users\Josiah\Downloads\JetBrainsMono-2.225\fonts\ttf\JetBrainsMono-Regular.ttf)";
#elif defined(__APPLE__)
    // TODO find apple font locations
#elif defined(linux)
    // TODO find linix font locatins
#endif

    projection = ortho(0.0f, screenWidth, 0.0f, screenHeight);
    createCommandPool();
    createDescriptorPool();
    createPipelineCache();
    createDescriptorSetLayout();
    createPipeline();
    createProjectionBuffer();
}

void Fonts::createTexture(FT_Face font, Texture &texture) {
    auto glyph = font->glyph;
    VkDeviceSize size = glyph->bitmap.width * glyph->bitmap.rows;
    VulkanBuffer stagingBuffer = device->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, size);
    stagingBuffer.copy(glyph->bitmap.buffer, size);

    VkImageCreateInfo imageCreateInfo = initializers::imageCreateInfo(
            VK_IMAGE_TYPE_2D,
            VK_FORMAT_R8_SRGB,
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            glyph->bitmap.width, glyph->bitmap.rows);

    texture.image = device->createImage(imageCreateInfo, VMA_MEMORY_USAGE_GPU_ONLY);
    texture.image.transitionLayout(commandPool, device->queues.graphics, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    commandPool.oneTimeCommand(device->queues.graphics, [&](auto cmdBuffer) {
        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {glyph->bitmap.width, glyph->bitmap.rows, 1u};

        vkCmdCopyBufferToImage(cmdBuffer, stagingBuffer, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                               &region);
    });

    texture.image.transitionLayout(commandPool, device->queues.graphics, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    VkImageSubresourceRange subresourceRange;
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount = 1;

    texture.imageView = texture.image.createView(VK_FORMAT_R8_SRGB, VK_IMAGE_VIEW_TYPE_2D, subresourceRange);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 16;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    texture.sampler = device->createSampler(samplerInfo);
}

std::string Fonts::getFontName(std::string_view name, FontStyle style) {
    std::string fName{name};
    if(integral_value(style) & integral_value(FontStyle::BOLD)){
        fName += " Bold";
    }
    if(integral_value(style) & integral_value(FontStyle::ITALIC)){
        fName += " Bold";
    }
    return fName;
}

void Fonts::createCommandPool() {
    commandPool = device->createCommandPool(*device->queueFamilyIndex.graphics);
}

void Fonts::createPipelineCache() {
    pipelineCache = device->createPipelineCache();
}

void Fonts::createDescriptorSetLayout() {
    std::vector<VkDescriptorSetLayoutBinding> bindings(3);

    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    bindings[2].binding = 2;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[2].descriptorCount = 1;
    bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    descriptorSetLayout = device->createDescriptorSetLayout(bindings);
}

void Fonts::createDescriptorPool() {
    std::vector<VkDescriptorPoolSize> poolSizes{
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 * MAX_SETS},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_SETS}
    };
    descriptorPool = device->createDescriptorPool(MAX_SETS, poolSizes);
}

std::vector<VkDescriptorSet>  Fonts::createDescriptorSets() {
    std::vector<VkDescriptorSetLayout> layouts(NUM_CHAR, descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = NUM_CHAR;
    allocInfo.pSetLayouts = layouts.data();

    std::vector<VkDescriptorSet> sets(NUM_CHAR);

    ASSERT(vkAllocateDescriptorSets(*device, &allocInfo, sets.data()));

    return sets;
}

void Fonts::createProjectionBuffer() {
    VkDeviceSize size = sizeof(glm::mat4);


    projectionBuffer = device->createBuffer(
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU,
            size);
    projectionBuffer.copy(glm::value_ptr(projection), size);
}

VulkanBuffer Fonts::createVertexBuffer() {
    VkDeviceSize size = sizeof(glm::vec4) * NUM_VERTICES;

    return device->createBuffer( VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU,
            size);
}

void Fonts::createPipeline() {

    ShaderModule vertexModule{ R"(..\..\data\shaders\font\font.vert.spv)", *device };
    ShaderModule fragmentModule{ R"(..\..\data\shaders\font\font.frag.spv)", *device };

    auto shaderStages = initializers::vertexShaderStages(
            {
                    { vertexModule, VK_SHADER_STAGE_VERTEX_BIT},
                    { fragmentModule, VK_SHADER_STAGE_FRAGMENT_BIT}
            }
    );

    std::vector<VkVertexInputBindingDescription> bindingDesc{ {0, sizeof(glm::vec4), VK_VERTEX_INPUT_RATE_VERTEX} };
    std::vector<VkVertexInputAttributeDescription> attributeDesc{ {0u, 0u, VK_FORMAT_R32G32B32A32_SFLOAT, 0u} };

    auto vertexInputState = initializers::vertexInputState( { bindingDesc }, { attributeDesc });

    auto inputAssemblyState = initializers::inputAssemblyState();
    inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

    auto viewport = initializers::viewport(screenWidth, screenHeight);
    auto scissor = initializers::scissor(screenWidth, screenHeight);

    auto viewportState = initializers::viewportState(viewport, scissor);

    auto rasterizationState = initializers::rasterizationState();
    rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationState.cullMode = VK_CULL_MODE_NONE;
    rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;

    auto multisampleState = initializers::multisampleState();   // TODO enable;

    auto depthStencilState = initializers::depthStencilState();
    depthStencilState.depthTestEnable = true;
    depthStencilState.depthWriteEnable = true;
    depthStencilState.depthCompareOp = VK_COMPARE_OP_ALWAYS;
    depthStencilState.minDepthBounds = 0.0f;
    depthStencilState.maxDepthBounds = 1.0f;

    VkPipelineColorBlendAttachmentState colorBlendAttachmentState{};
    colorBlendAttachmentState.blendEnable = VK_TRUE;
    colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachmentState.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT
            | VK_COLOR_COMPONENT_G_BIT
            | VK_COLOR_COMPONENT_B_BIT
            | VK_COLOR_COMPONENT_A_BIT;
    auto colorBlendState = initializers::colorBlendState(colorBlendAttachmentState);

    pipelineLayout = device->createPipelineLayout({ descriptorSetLayout });

    VkGraphicsPipelineCreateInfo createInfo = initializers::pipelineCreateInfo();
    createInfo.stageCount = COUNT(shaderStages);
    createInfo.pStages = shaderStages.data();
    createInfo.pVertexInputState = &vertexInputState;
    createInfo.pInputAssemblyState = &inputAssemblyState;
    createInfo.pViewportState = &viewportState;
    createInfo.pRasterizationState = &rasterizationState;
    createInfo.pMultisampleState = &multisampleState;
    createInfo.pDepthStencilState = &depthStencilState;
    createInfo.pColorBlendState = &colorBlendState;
    createInfo.layout = pipelineLayout;
    createInfo.renderPass = *renderPass;
    createInfo.subpass = subpass;

    pipeline = device->createGraphicsPipeline(createInfo, pipelineCache);
}

void Fonts::refresh(int width, int height, VulkanRenderPass* newRenderPass) {
    renderPass = newRenderPass;
    dispose(pipelineLayout);
    dispose(pipeline);
    dispose(descriptorPool);
    createDescriptorPool();
    createPipeline();
    for(auto& [_, font] : fonts){
        font.refresh();
    }
    screenWidth = static_cast<float>(width);
    screenHeight = static_cast<float>(height);
    projection = glm::ortho(0.0f, screenWidth, 0.0f, screenHeight);
}

void Fonts::subPassIndex(const uint32_t index) {
    subpass = index;
}

void Fonts::setWordSpacing(float spacing) {
    worldSpacing = spacing;
}

void Fonts::cleanup() {
    for(auto& [_, font] : fonts){
        font.~Font();
    }
    dispose(projectionBuffer);
    dispose(descriptorSetLayout);
    dispose(pipelineLayout);
    dispose(pipeline);
    dispose(pipelineCache);
    dispose(descriptorPool);
    dispose(commandPool);
    FT_Done_FreeType(ftLibrary);
}

glm::mat4 Fonts::projection;
std::map<std::string, std::string> Fonts::location{};
FT_Library  Fonts::ftLibrary;
std::map<std::string, Font> Fonts::fonts{};

VulkanDevice* Fonts::device;
VulkanDescriptorSetLayout Fonts::descriptorSetLayout;
VulkanPipelineLayout Fonts::pipelineLayout;
VulkanPipelineCache Fonts::pipelineCache;
VulkanPipeline Fonts::pipeline;
VulkanDescriptorPool Fonts::descriptorPool;
VulkanRenderPass* Fonts::renderPass;
VulkanCommandPool Fonts::commandPool;
VulkanBuffer Fonts::projectionBuffer;

uint32_t Fonts::subpass = 0;
float Fonts::screenWidth = 0;
float Fonts::screenHeight = 0;
int Fonts::numFonts = MAX_FONTS;
float Fonts::worldSpacing = DEFAULT_WORD_SPACE;
uint32_t Fonts::swapChainImageCount = 0;
uint32_t* Fonts::currentImageIndex = nullptr;