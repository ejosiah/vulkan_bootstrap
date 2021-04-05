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
#define JET_BRAINS_MONO "JetBrains Mono Regular"

static constexpr int NUM_VERTICES = 4;
static constexpr int NUM_CHAR = 128;
static constexpr int MAX_FONTS = 20;
static constexpr uint32_t MAX_SETS = NUM_CHAR * MAX_FONTS;
static constexpr int MAX_TEXT_SIZE = 10000;
static constexpr float DEFAULT_WORD_SPACE = 0.5f;


struct Character{
    char value;
    VkDescriptorSet descriptorSet;
    Texture texture;
    glm::ivec2 size;
    glm::ivec2 bearing;
    uint32_t advance;
    bool loaded = false;
};

struct CharacterInstance{
    Character* character;
    VulkanBuffer buffer;
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
    Font(std::string_view name, uint32_t swapChainImageCount, uint32_t* index, int size = 10, FontStyle style = FontStyle::NORMAL, const glm::vec3& color = glm::vec3(1));

    void load();

    void createDescriptorSets();

    void createColorBuffer();

public:
    DISABLE_COPY(Font);

    Font() = default;

    Font(Font&& source) noexcept;

    ~Font(){
        for(auto& ch : characters){
            dispose(ch.descriptorSet);
            dispose(ch.texture.imageView);
            dispose(ch.texture.sampler);
            dispose(ch.texture.image);
        }
        dispose(colorBuffer);
    }

    Font& operator=(Font&& source) noexcept;

    void write(const std::string& text, float x, float y, float d = -1);

    void clear();

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
    mutable std::vector<std::vector<CharacterInstance>> texts;
    VulkanBuffer colorBuffer;
    uint32_t* currentImageIndex = nullptr;
    uint32_t maxHeight = std::numeric_limits<uint32_t>::min();
    uint32_t maxWidth = std::numeric_limits<uint32_t>::min();

};

class Fonts{
    friend class Font;
public:
    static Font* getFont(std::string_view name, int size, FontStyle style = FontStyle::NORMAL, const glm::vec3& color = glm::vec3(0));

    static void init(VulkanDevice* device, VulkanRenderPass* rPass, uint32_t rSubpass, uint32_t swapChainImageCount, uint32_t* index, int width, int height);

    static void refresh(int width, int height, VulkanRenderPass* newRenderPass);

    static void createTexture(FT_Face font, Texture& texture);

    static void subPassIndex(uint32_t index);

    static void setWordSpacing(float spacing);

    static void updateProjection();

    static void cleanup();

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

private:
    static glm::mat4 projection;
    static std::map<std::string, std::string> location;
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
    static float worldSpacing;
    static uint32_t swapChainImageCount;
    static uint32_t* currentImageIndex;
};