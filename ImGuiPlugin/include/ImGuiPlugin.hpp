#pragma once

#include "common.h"
//#ifdef _WIN32
//#undef APIENTRY
//#define GLFW_EXPOSE_NATIVE_WIN32
//#include <GLFW/glfw3native.h>   // for glfwGetWin32Window
//#endif
#include "Plugin.hpp"
#include "Texture.h"
#include "imgui.h"

#define IM_GUI_PLUGIN "Dear ImGui Plugin"
#define IMGUI_NO_WINDOW ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMouseInputs | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground

struct FontInfo{
    std::string name;
    fs::path path;
    float pixelSize;
};

struct FrameRenderBuffers{
    VulkanBuffer vertices;
    VulkanBuffer indices;
};

struct FontInfoComp{
    std::less<std::string> less = std::less<std::string>{};

    bool operator()(const FontInfo& a, const FontInfo& b) const{
        auto str_a = a.name + std::to_string(a.pixelSize);
        auto str_b = b.name + std::to_string(b.pixelSize);

        return less(str_a, str_b);
    };
};

class ImGuiPlugin final : public Plugin{
public:
    explicit ImGuiPlugin(std::vector<FontInfo> fonts = {}, uint32_t subpass = 0);

    ~ImGuiPlugin() final;

    [[nodiscard]]
    std::string name() const final;

    void init() final;

    void initWindowRenderBuffers();

    void createDeviceObjects();

    void createFontSampler();

    void createDescriptorSetLayout();

    void createDescriptorPool();

    void createDescriptorSet();

    void createPipeline();

    void createPipelineLayout();

    void mapInputs();

    void loadFonts();

    ImTextureID addTexture(Texture& texture);

    ImTextureID addTexture(VulkanImageView& imageView);

    ImFont* font(const std::string& name, float pixelSize);

    void newFrame() final;

    void updateMouse();

    void updateMouseCursor();

    void cleanup() final;

    void onSwapChainDispose() final;

    void onSwapChainRecreation() final;

    void draw(VkCommandBuffer commandBuffer) final;

    void setupRenderState(FrameRenderBuffers* rb, ImDrawData* draw_data, VkCommandBuffer command_buffer, int fb_width, int fb_height, VkDescriptorSet aDescriptorSet = nullptr);

    MousePressListener mousePressListener() final;

    MouseWheelMovedListener mouseWheelMoveListener() final;

    KeyPressListener keyPressListener() final;

    KeyReleaseListener keyReleaseListener() final;

    WindowResizeListener windowResizeListener() final;

    static void checkKeyMods(ImGuiIO& io, const KeyEvent& event);

    // FIXME replace with listener
    static const char* getClipboardText(void* userData);

    // FIXME replace with listener
    static void setClipboardText(void* userData, const char* text);

    // FIXME replace with listener
    static void charCallBack(GLFWwindow* window, uint32_t c);

    static std::map<FontInfo, ImFont*, FontInfoComp>  setFonts(const std::vector<FontInfo>& fontInfos);

protected:
    void bindDescriptorSet(VkCommandBuffer command_buffer, VkDescriptorSet aDescriptorSet = nullptr);

    VulkanPipelineCache pipelineCache;
    Texture fontTexture;
    struct {
        uint32_t index;
        uint32_t count;
        std::vector<FrameRenderBuffers> frameRenderBuffers;
    } windowRenderBuffers;

    VulkanDescriptorSetLayout descriptorSetLayout;
    VulkanDescriptorPool descriptorPool;
    VulkanPipelineLayout pipelineLayout;
    VulkanPipeline pipeline;
    std::map<FontInfo, ImFont*, FontInfoComp> fonts;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> descriptorSets;

    struct {
        std::array<bool, ImGuiMouseButton_COUNT> justPressed;
        std::array<GLFWcursor *, ImGuiMouseCursor_COUNT> cursors;
    } mouse{};

    double time = 0.0;
    uint32_t subpass;
    bool drawRequested = true;

};