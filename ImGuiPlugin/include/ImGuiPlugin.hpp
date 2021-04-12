#pragma once

#include <common.h>
//#ifdef _WIN32
//#undef APIENTRY
//#define GLFW_EXPOSE_NATIVE_WIN32
//#include <GLFW/glfw3native.h>   // for glfwGetWin32Window
//#endif
#include <Plugin.hpp>
#include <Texture.h>
#include <imgui.h>

#define IM_GUI_PLUGIN "Dear ImGui Plugin"

struct FontInfo{
    fs::path path;
    float pixelSize;
};

struct FrameRenderBuffers{
    VulkanBuffer vertices;
    VulkanBuffer indices;
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

    void newFrame() final;

    void updateMouse();

    void updateMouseCursor();

    void cleanup() final;

    void onSwapChainDispose() final;

    void onSwapChainRecreation() final;

    void onDraw(VkCommandBuffer commandBuffer) final;

    void setupRenderState(FrameRenderBuffers* rb, ImDrawData* draw_data, VkCommandBuffer command_buffer, int fb_width, int fb_height);

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

protected:
    VulkanDescriptorPool descriptorPool;
    VulkanDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    VulkanPipelineLayout pipelineLayout;
    VulkanPipeline pipeline;
    VulkanPipelineCache pipelineCache;
    std::vector<FontInfo> fonts;
    Texture fontTexture;

    struct {
        std::array<bool, ImGuiMouseButton_COUNT> justPressed;
        std::array<GLFWcursor *, ImGuiMouseCursor_COUNT> cursors;
    } mouse{};



    struct {
        uint32_t index;
        uint32_t count;
        std::vector<FrameRenderBuffers> frameRenderBuffers;
    } windowRenderBuffers;

    double time = 0.0;
    uint32_t subpass;
};