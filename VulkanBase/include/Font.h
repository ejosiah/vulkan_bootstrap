#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H
#include <glm/glm.hpp>
#include "common.h"
#include "Texture.h"
#include "VulkanDevice.h"

#define COURIER "Courier"
#define ARIAL "Arial"
#define ARIAL_BOLD "Arial Bold"
#define ARIAL_BOLD_ITALIC "Arial Bold Italic"
#define ARIAL_ITALIC "Arial Italic"

static constexpr int NUM_VERTICES = 4;
static constexpr int NUM_CHAR = 128;
static constexpr int MAX_FONTS = 100;
static constexpr uint32_t MAX_SETS = NUM_CHAR * MAX_FONTS;


struct Character{
    char value;
    VulkanBuffer buffer;
    VkDescriptorSet descriptorSet;
    Texture texture;
    glm::ivec2 size;
    glm::ivec2 bearing;
    uint32_t advance;
    bool hot = false;
};

enum class FontStyle : uint32_t {
    NORMAL = 0,
    BOLD = 1,
    ITALIC = 2
};

inline uint32_t integral_value(FontStyle style){
    return static_cast<uint32_t>(style);
}

class Font{
friend class Fonts;
private:
    Font(std::string_view name, int size = 10, FontStyle style = FontStyle::NORMAL, const glm::vec3& color = glm::vec3(1));

    void load();

    void createDescriptorSets();

    void createColorBuffer();

public:
    DISABLE_COPY(Font);

    Font() = default;

    Font(Font&& source) noexcept;

    Font& operator=(Font&& source) noexcept;

    void write(const std::string& text, float x, float y, float d = -1) const;

    void draw(VkCommandBuffer commandBuffer) const;

    [[nodiscard]]
    glm::ivec2 sizeOf(const std::string& text) const;

    void refresh();

private:
    int size = 10;
    FontStyle style = FontStyle::NORMAL;
    glm::vec3 color = glm::vec3(0);    // TODO do we need this?
    FT_Face face = nullptr;
    mutable std::array<Character, NUM_CHAR> characters;
    VulkanBuffer colorBuffer;

    uint32_t maxHeight = std::numeric_limits<uint32_t>::min();
    uint32_t maxWidth = std::numeric_limits<uint32_t>::min();

};

class Fonts{
    friend class Font;
public:
    static const Font& getFont(std::string_view name, int size, FontStyle style, const glm::vec3& color);

    static void init(VulkanDevice* device, int width, int height);

    static void refresh(int width, int height, VulkanRenderPass* newRenderPass);

    static void createTexture(FT_Face font, Texture& texture);

private:
    static std::string getFontName(std::string_view name, FontStyle style);

    static void createCommandPool();

    static void createPipelineCache();

    static void createDescriptorSetLayout();

    static void createDescriptorPool();

    static std::vector<VkDescriptorSet> createDescriptorSets();

    static void createProjectionBuffer();

    static VulkanBuffer createVertexBuffer();

    static void createPipeline();

    static void subPassIndex(uint32_t index);

private:
    static glm::mat4 projection;
    static std::map<std::string, std::string> fontLocation;
    static FT_Library ftLibrary;
    static std::map<std::string, Font> fonts;

    static VulkanDevice* device;
    static VulkanDescriptorSetLayout descriptorSetLayout;
    static VulkanPipelineLayout pipelineLayout;
    static VulkanPipelineCache pipelineCache;
    static VulkanPipeline pipeline;
    static VulkanDescriptorPool descriptorPool;
    static VulkanRenderPass* renderPass;
    static VulkanCommandPool commandPool;
    static VulkanBuffer projectionBuffer;
    static uint32_t subpass;
    static float screenWidth;
    static float screenHeight;
    static int numFonts;
};