#include "ImGuiPlugin.hpp"
#include "ImGuiShaders.hpp"
#include <VulkanShaderModule.h>
#include <fmt/format.h>

ImGuiPlugin::ImGuiPlugin(std::vector<FontInfo> fontInfos, uint32_t subpass)
: fonts(setFonts(fontInfos))
, subpass(subpass)
{

}

ImGuiPlugin::~ImGuiPlugin() {
    for(auto i = 0; i < ImGuiMouseCursor_COUNT; i++){
        glfwDestroyCursor(mouse.cursors[i]);
        mouse.cursors[i] = nullptr;
    }
    ImGui::DestroyContext();
}

std::string ImGuiPlugin::name() const {
    return IM_GUI_PLUGIN;
}

void ImGuiPlugin::init() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    initWindowRenderBuffers();
    mapInputs();
    createDeviceObjects();
    loadFonts();
}

void ImGuiPlugin::initWindowRenderBuffers() {
    windowRenderBuffers.frameRenderBuffers.resize(data.swapChain->imageCount());
}

void ImGuiPlugin::createDeviceObjects(){
    pipelineCache = data.device->createPipelineCache();
    createFontSampler();
    createDescriptorSetLayout();
    createDescriptorPool();
    createDescriptorSet();
    createPipeline();
}

void ImGuiPlugin::createFontSampler() {
    auto sampleCreateInfo = initializers::samplerCreateInfo();
    sampleCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampleCreateInfo.magFilter = VK_FILTER_LINEAR;
    sampleCreateInfo.minFilter = VK_FILTER_LINEAR;
    sampleCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampleCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampleCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampleCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampleCreateInfo.minLod = -1000;
    sampleCreateInfo.maxLod = 1000;
    sampleCreateInfo.maxAnisotropy = 1.0f;

    fontTexture.sampler = data.device->createSampler(sampleCreateInfo);
}

void ImGuiPlugin::createDescriptorSetLayout() {
    VkSampler sampler[1] = {fontTexture.sampler};
    std::vector<VkDescriptorSetLayoutBinding> binding(1);
    binding[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding[0].descriptorCount = 1;
    binding[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    binding[0].pImmutableSamplers = sampler;

    descriptorSetLayout = data.device->createDescriptorSetLayout(binding);
}

void ImGuiPlugin::createDescriptorPool() {
    std::vector<VkDescriptorPoolSize> pool_sizes =   {{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100}};
    descriptorPool = data.device->createDescriptorPool(COUNT(pool_sizes) * 10, pool_sizes, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);
}


void ImGuiPlugin::createDescriptorSet() {
    assert(descriptorPool.pool);
    assert(descriptorSetLayout.handle);

    descriptorSet = descriptorPool.allocate({ descriptorSetLayout }).front();
}

void ImGuiPlugin::createPipeline() {
    VulkanShaderModule vertexShaderModule{
        std::vector<uint32_t>{std::begin(__glsl_shader_vert_spv), std::end(__glsl_shader_vert_spv)}, data.device->logicalDevice};
    VulkanShaderModule fragShaderModule{
        std::vector<uint32_t>{std::begin(__glsl_shader_frag_spv), std::end(__glsl_shader_frag_spv)}, data.device->logicalDevice};

    auto stage = initializers::vertexShaderStages({
        {vertexShaderModule, VK_SHADER_STAGE_VERTEX_BIT},
        {fragShaderModule,   VK_SHADER_STAGE_FRAGMENT_BIT}
    });

    std::vector<VkVertexInputBindingDescription> binding_desc(1);
    binding_desc[0].stride = sizeof(ImDrawVert);
    binding_desc[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::vector<VkVertexInputAttributeDescription> attribute_desc(3);
    attribute_desc[0].location = 0;
    attribute_desc[0].binding = binding_desc[0].binding;
    attribute_desc[0].format = VK_FORMAT_R32G32_SFLOAT;
    attribute_desc[0].offset = IM_OFFSETOF(ImDrawVert, pos);
    attribute_desc[1].location = 1;
    attribute_desc[1].binding = binding_desc[0].binding;
    attribute_desc[1].format = VK_FORMAT_R32G32_SFLOAT;
    attribute_desc[1].offset = IM_OFFSETOF(ImDrawVert, uv);
    attribute_desc[2].location = 2;
    attribute_desc[2].binding = binding_desc[0].binding;
    attribute_desc[2].format = VK_FORMAT_R8G8B8A8_UNORM;
    attribute_desc[2].offset = IM_OFFSETOF(ImDrawVert, col);

    auto vertex_info = initializers::vertexInputState(binding_desc, attribute_desc);

    auto ia_info = initializers::inputAssemblyState();

    auto viewport_info = initializers::viewportState();
    viewport_info.viewportCount = 1;
    viewport_info.scissorCount = 1;

    auto raster_info = initializers::rasterizationState();
    raster_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster_info.polygonMode = VK_POLYGON_MODE_FILL;
    raster_info.cullMode = VK_CULL_MODE_NONE;
    raster_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster_info.lineWidth = 1.0f;

    auto ms_info = initializers::multisampleState();
    ms_info.rasterizationSamples =  data.msaaSamples;

    std::vector<VkPipelineColorBlendAttachmentState> color_attachment(1);
    color_attachment[0].blendEnable = VK_TRUE;
    color_attachment[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_attachment[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_attachment[0].colorBlendOp = VK_BLEND_OP_ADD;
    color_attachment[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_attachment[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_attachment[0].alphaBlendOp = VK_BLEND_OP_ADD;
    color_attachment[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    auto depth_info = initializers::depthStencilState();    // TODO check this;

    auto blend_info = initializers::colorBlendState(color_attachment);

    std::vector<VkDynamicState> dynamic_states{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamic_state = initializers::dynamicState(dynamic_states);

    createPipelineLayout();

    auto info = initializers::graphicsPipelineCreateInfo();
    info.stageCount = COUNT(stage);
    info.pStages = stage.data();
    info.pVertexInputState = &vertex_info;
    info.pInputAssemblyState = &ia_info;
    info.pViewportState = &viewport_info;
    info.pRasterizationState = &raster_info;
    info.pMultisampleState = &ms_info;
    info.pDepthStencilState = &depth_info;
    info.pColorBlendState = &blend_info;
    info.pDynamicState = &dynamic_state;
    info.layout = pipelineLayout;
    info.renderPass = *data.renderPass;
    info.subpass = subpass;
    pipeline = data.device->createGraphicsPipeline(info, pipelineCache);
}

void ImGuiPlugin::createPipelineLayout() {
    dispose(pipelineLayout);
    // Constants: we are using 'vec2 offset' and 'vec2 scale' instead of a full 3d projection matrix
    std::vector<VkPushConstantRange> push_constants(1);
    push_constants[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    push_constants[0].offset = sizeof(float) * 0;
    push_constants[0].size = sizeof(float) * 4;
    std::vector<VkDescriptorSetLayout> set_layout{ descriptorSetLayout };

    pipelineLayout = data.device->createPipelineLayout(set_layout, push_constants);
}


void ImGuiPlugin::mapInputs() {
    auto window = data.window;
    auto io = ImGui::GetIO();
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;         // We can honor GetMouseCursor() values (optional)
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;          // We can honor io.WantSetMousePos requests (optional, rarely used)
    io.BackendPlatformName = "imgui_impl_glfw";

    // Keyboard mapping. Dear ImGui will use those indices to peek into the io.KeysDown[] array.
    io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
    io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = GLFW_KEY_PAGE_UP;
    io.KeyMap[ImGuiKey_PageDown] = GLFW_KEY_PAGE_DOWN;
    io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
    io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
    io.KeyMap[ImGuiKey_Insert] = GLFW_KEY_INSERT;
    io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
    io.KeyMap[ImGuiKey_Space] = GLFW_KEY_SPACE;
    io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
    io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
    io.KeyMap[ImGuiKey_KeyPadEnter] = GLFW_KEY_KP_ENTER;
    io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
    io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
    io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
    io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
    io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
    io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;

    io.SetClipboardTextFn = setClipboardText;
    io.GetClipboardTextFn = getClipboardText;
    io.ClipboardUserData = window;
//#if defined(_WIN32)
//    io.ImeWindowHandle = (void*)glfwGetWin32Window(window);
//#endif

    // Create mouse cursors
    // (By design, on X11 cursors are user configurable and some cursors may be missing. When a cursor doesn't exist,
    // GLFW will emit an error which will often be printed by the app, so we temporarily disable error reporting.
    // Missing cursors will return nullptr and our _UpdateMouseCursor() function will use the Arrow cursor instead.)
    GLFWerrorfun prev_error_callback = glfwSetErrorCallback(nullptr);
    mouse.cursors[ImGuiMouseCursor_Arrow] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    mouse.cursors[ImGuiMouseCursor_TextInput] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
    mouse.cursors[ImGuiMouseCursor_ResizeNS] = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
    mouse.cursors[ImGuiMouseCursor_ResizeEW] = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
    mouse.cursors[ImGuiMouseCursor_Hand] = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
#if GLFW_HAS_NEW_CURSORS
    mouse.cursors[ImGuiMouseCursor_ResizeAll] = glfwCreateStandardCursor(GLFW_RESIZE_ALL_CURSOR);
    mouse.cursors[ImGuiMouseCursor_ResizeNESW] = glfwCreateStandardCursor(GLFW_RESIZE_NESW_CURSOR);
    mouse.cursors[ImGuiMouseCursor_ResizeNWSE] = glfwCreateStandardCursor(GLFW_RESIZE_NWSE_CURSOR);
    mouse.cursors[ImGuiMouseCursor_NotAllowed] = glfwCreateStandardCursor(GLFW_NOT_ALLOWED_CURSOR);
#else
    mouse.cursors[ImGuiMouseCursor_ResizeAll] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    mouse.cursors[ImGuiMouseCursor_ResizeNESW] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    mouse.cursors[ImGuiMouseCursor_ResizeNWSE] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    mouse.cursors[ImGuiMouseCursor_NotAllowed] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
#endif
    glfwSetErrorCallback(prev_error_callback);

}

void ImGuiPlugin::newFrame() {
    if(!drawRequested) return;
    auto& io = ImGui::GetIO();

    IM_ASSERT(io.Fonts->IsBuilt() && "Font atlas not built! It is generally built by the renderer backend. Missing call to renderer _NewFrame() function? e.g. ImGui_ImplOpenGL3_NewFrame().");

    // Setup display size (every frame to accommodate for window resizing)
    int w, h;
    int display_w, display_h;
    glfwGetWindowSize(data.window, &w, &h);
    glfwGetFramebufferSize(data.window, &display_w, &display_h);
    io.DisplaySize = ImVec2((float)w, (float)h);
    if (w > 0 && h > 0)
        io.DisplayFramebufferScale = ImVec2((float)display_w / w, (float)display_h / h);

    // Setup time step
    double current_time = glfwGetTime();
    io.DeltaTime = time > 0.0 ? (float)(current_time - time) : (float)(1.0f / 60.0f);
    time = current_time;

    updateMouse();
    updateMouseCursor();
    ImGui::NewFrame();
    drawRequested = false;
}

void ImGuiPlugin::updateMouse() {
    auto &io = ImGui::GetIO();

    for (int i = 0; i < std::size(io.MouseDown); i++) {
        // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
        io.MouseDown[i] = mouse.justPressed[i] || glfwGetMouseButton(data.window, i) != 0;
        mouse.justPressed[i] = false;
    }

    // Update mouse position
    const ImVec2 mouse_pos_backup = io.MousePos;
    io.MousePos = ImVec2(-MIN_FLOAT, -MAX_FLOAT);
#ifdef __EMSCRIPTEN__
    const bool focused = true; // Emscripten
#else
    const bool focused = glfwGetWindowAttrib(data.window, GLFW_FOCUSED) != 0;
#endif
    if (focused) {
        if (io.WantSetMousePos) {
            glfwSetCursorPos(data.window, (double) mouse_pos_backup.x, (double) mouse_pos_backup.y);
        } else {
            double mouse_x, mouse_y;
            glfwGetCursorPos(data.window, &mouse_x, &mouse_y);
            io.MousePos = ImVec2((float) mouse_x, (float) mouse_y);

        }
    }
}

void ImGuiPlugin::updateMouseCursor() {
    auto& io = ImGui::GetIO();
    if ((io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) || glfwGetInputMode(data.window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
        return;

    ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
    if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor){
        // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
        glfwSetInputMode(data.window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    }
    else{
        // Show OS mouse cursor
        // FIXME-PLATFORM: Unfocused windows seems to fail changing the mouse cursor with GLFW 3.2, but 3.3 works here.
        glfwSetCursor(data.window, mouse.cursors[imgui_cursor] ? mouse.cursors[imgui_cursor] : mouse.cursors[ImGuiMouseCursor_Arrow]);
        glfwSetInputMode(data.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

void ImGuiPlugin::cleanup() {
}

void ImGuiPlugin::onSwapChainDispose() {
    dispose(pipeline);
}

void ImGuiPlugin::onSwapChainRecreation() {
    createPipeline();
}


void ImGuiPlugin::loadFonts() {
    auto io = ImGui::GetIO();
    io.Fonts->AddFontDefault();
    for(auto [fontInfo, _] : fonts){
        fonts[fontInfo] = io.Fonts->AddFontFromFileTTF(fontInfo.path.string().c_str(), fontInfo.pixelSize);
    }

    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    textures::create(*data.device, fontTexture, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM
                     , pixels, {width, height, 1}, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, sizeof(char));

    std::array<VkDescriptorImageInfo, 1> desc_image{
            {fontTexture.sampler, fontTexture.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}
    };

    std::array<VkWriteDescriptorSet, 1> write_desc{};
    write_desc[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_desc[0].dstSet = descriptorSet;
    write_desc[0].descriptorCount = COUNT(desc_image);
    write_desc[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write_desc[0].pImageInfo = desc_image.data();
    vkUpdateDescriptorSets(*data.device, COUNT(write_desc), write_desc.data(), 0, nullptr);
}

MousePressListener ImGuiPlugin::mousePressListener() {
    return [&](const MouseEvent& event){
        int button = static_cast<int>(event.button);
        if(button >= 0 && button < ImGuiMouseButton_COUNT){
            mouse.justPressed[button] = true;
        }
    };
}


MouseWheelMovedListener ImGuiPlugin::mouseWheelMoveListener() {
    return [](const MouseEvent& event){
        auto& io = ImGui::GetIO();
        io.MouseWheelH = event.scrollOffset.x;
        io.MouseWheel = event.scrollOffset.y;
    };
}

KeyPressListener ImGuiPlugin::keyPressListener() {
    return [](const KeyEvent& event){
        auto& io = ImGui::GetIO();
        int key = static_cast<int>(event.key);
        io.KeysDown[key] = true;
        checkKeyMods(io, event);
    };
}

KeyReleaseListener ImGuiPlugin::keyReleaseListener() {
    return [](const KeyEvent& event){
        auto& io = ImGui::GetIO();
        int key = static_cast<int>(event.key);
        io.KeysDown[key] = false;
        checkKeyMods(io, event);
    };
}

WindowResizeListener ImGuiPlugin::windowResizeListener() {
    return Plugin::windowResizeListener();
}

const char* ImGuiPlugin::getClipboardText(void* userData){
    return glfwGetClipboardString((GLFWwindow*)userData);;
}

void ImGuiPlugin::setClipboardText(void* userData, const char* text){
    glfwSetClipboardString((GLFWwindow*)userData, text);
}

void ImGuiPlugin::charCallBack(GLFWwindow* window, uint32_t c) {
    auto& io = ImGui::GetIO();
    io.AddInputCharacter(c);
}

void ImGuiPlugin::checkKeyMods(ImGuiIO& io, const KeyEvent &event) {
    io.KeyCtrl = io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
    io.KeyShift = io.KeysDown[GLFW_KEY_LEFT_SHIFT] || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
    io.KeyAlt = io.KeysDown[GLFW_KEY_LEFT_ALT] || io.KeysDown[GLFW_KEY_RIGHT_ALT];
#ifdef _WIN32
    io.KeySuper = false;
#else
    io.KeySuper = io.KeysDown[GLFW_KEY_LEFT_SUPER] || io.KeysDown[GLFW_KEY_RIGHT_SUPER];
#endif
}

void ImGuiPlugin::draw(VkCommandBuffer command_buffer) {
    drawRequested = true;
    ImGui::Render();
    auto draw_data = ImGui::GetDrawData();
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    int fb_width = (int) (draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int fb_height = (int) (draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0)
        return;


    auto *rb = &windowRenderBuffers.frameRenderBuffers[*data.currentImageIndex];

    if (draw_data->TotalVtxCount > 0) {
        // Create or resize the vertex/index buffers
        size_t vertex_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
        size_t index_size = draw_data->TotalIdxCount * sizeof(ImDrawIdx);
        if (rb->vertices.buffer == VK_NULL_HANDLE || rb->vertices.size < vertex_size) {
//            CreateOrResizeBuffer(rb->VertexBuffer, rb->VertexBufferMemory, rb->VertexBufferSize, vertex_size,
//                                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
            rb->vertices = data.device->createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU,
                                                     vertex_size);
        }
        if (rb->indices.buffer == VK_NULL_HANDLE || rb->indices.size < index_size) {
//            CreateOrResizeBuffer(rb->IndexBuffer, rb->IndexBufferMemory, rb->IndexBufferSize, index_size,
//                                 VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
            rb->indices = data.device->createBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU,
                                                    vertex_size);
        }

        // Upload vertex/index data into a single contiguous GPU buffer
        rb->vertices.map<ImDrawVert>([&](ImDrawVert *vtx_dst) {
            rb->indices.map<ImDrawIdx>([&](ImDrawIdx *idx_dst) {
                for (int n = 0; n < draw_data->CmdListsCount; n++) {
                    const ImDrawList *cmd_list = draw_data->CmdLists[n];
                    memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
                    memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
                    vtx_dst += cmd_list->VtxBuffer.Size;
                    idx_dst += cmd_list->IdxBuffer.Size;
                }
                std::array<VmaAllocation, 2> allocations{rb->vertices.allocation, rb->indices.allocation};
                std::array<VkDeviceSize, 2> offsets{0, 0};
                std::array<VkDeviceSize, 2> sizes{VK_WHOLE_SIZE, VK_WHOLE_SIZE};
                vmaFlushAllocations(data.device->allocator, 2, allocations.data(), offsets.data(), sizes.data());
            });
        });

    }

    // Setup desired Vulkan state
    setupRenderState(rb, draw_data, command_buffer, fb_width, fb_height);

    // Will project scissor/clipping rectangles into framebuffer space
    ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
    ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

    // Render command lists
    // (Because we merged all buffers into a single one, we maintain our own offset into them)
    int global_vtx_offset = 0;
    int global_idx_offset = 0;
    for (int n = 0; n < draw_data->CmdListsCount; n++) {
        const ImDrawList *cmd_list = draw_data->CmdLists[n];
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
            const ImDrawCmd *pcmd = &cmd_list->CmdBuffer[cmd_i];
            auto aDescriptorSet = reinterpret_cast<VkDescriptorSet>(pcmd->TextureId);
            bindDescriptorSet(command_buffer, aDescriptorSet);
            if (pcmd->UserCallback != nullptr) {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                    setupRenderState(rb, draw_data, command_buffer, fb_width, fb_height, aDescriptorSet);
                else
                    pcmd->UserCallback(cmd_list, pcmd);
            } else {
                // Project scissor/clipping rectangles into framebuffer space
                ImVec4 clip_rect;
                clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
                clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
                clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
                clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;

                if (clip_rect.x < fb_width && clip_rect.y < fb_height && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f) {
                    // Negative offsets are illegal for vkCmdSetScissor
                    if (clip_rect.x < 0.0f)
                        clip_rect.x = 0.0f;
                    if (clip_rect.y < 0.0f)
                        clip_rect.y = 0.0f;

                    // Apply scissor/clipping rectangle
                    VkRect2D scissor;
                    scissor.offset.x = (int32_t) (clip_rect.x);
                    scissor.offset.y = (int32_t) (clip_rect.y);
                    scissor.extent.width = (uint32_t) (clip_rect.z - clip_rect.x);
                    scissor.extent.height = (uint32_t) (clip_rect.w - clip_rect.y);
                    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

                    // Draw
                    vkCmdDrawIndexed(command_buffer, pcmd->ElemCount, 1, pcmd->IdxOffset + global_idx_offset,
                                     pcmd->VtxOffset + global_vtx_offset, 0);
                }
            }
        }
        global_idx_offset += cmd_list->IdxBuffer.Size;
        global_vtx_offset += cmd_list->VtxBuffer.Size;
    }
}

void ImGuiPlugin::setupRenderState(FrameRenderBuffers* rb, ImDrawData* draw_data, VkCommandBuffer command_buffer, int fb_width, int fb_height, VkDescriptorSet aDescriptorSet) {
    // Bind pipeline and descriptor sets:
    {
        bindDescriptorSet(command_buffer);
    }

    // Bind Vertex And Index Buffer:
    if (draw_data->TotalVtxCount > 0)
    {
        VkBuffer vertex_buffers[1] = { rb->vertices };
        VkDeviceSize vertex_offset[1] = { 0 };
        vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, vertex_offset);
        vkCmdBindIndexBuffer(command_buffer, rb->indices, 0, sizeof(ImDrawIdx) == 2 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
    }

    // Setup viewport:
    {
        VkViewport viewport;
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = (float)fb_width;
        viewport.height = (float)fb_height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(command_buffer, 0, 1, &viewport);
    }

    // Setup scale and translation:
    // Our visible imgui space lies from draw_data->DisplayPps (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
    {
        float scale[2];
        scale[0] = 2.0f / draw_data->DisplaySize.x;
        scale[1] = 2.0f / draw_data->DisplaySize.y;
        float translate[2];
        translate[0] = -1.0f - draw_data->DisplayPos.x * scale[0];
        translate[1] = -1.0f - draw_data->DisplayPos.y * scale[1];
        vkCmdPushConstants(command_buffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(float) * 0, sizeof(float) * 2, scale);
        vkCmdPushConstants(command_buffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(float) * 2, sizeof(float) * 2, translate);
    }
}

std::map<FontInfo, ImFont *, FontInfoComp> ImGuiPlugin::setFonts(const std::vector<FontInfo> &fontInfos) {
    std::map<FontInfo, ImFont *, FontInfoComp> fonts;
    for(const auto& info : fontInfos){
        fonts.insert(std::make_pair(info, nullptr));
    }
    return fonts;
}

ImFont *ImGuiPlugin::font(const std::string& name, float pixelSize) {
    auto key = name + std::to_string(pixelSize);
    for(auto& [fontInfo, font] : fonts){
        if(fontInfo.name == name  && fontInfo.pixelSize == pixelSize){
            return font;
        }
    }
    throw std::runtime_error{fmt::format("requested font: {}, size: {}, was not previously loaded", name, pixelSize)};
}

ImTextureID ImGuiPlugin::addTexture(Texture& texture) {
    auto aDescriptorSet = descriptorPool.allocate({ descriptorSetLayout}).front();
    descriptorSets.push_back(aDescriptorSet);

    auto writes = initializers::writeDescriptorSets<1>();
    auto& write = writes[0];
    write.dstSet = aDescriptorSet;
    write.dstBinding = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.descriptorCount = 1;
    VkDescriptorImageInfo imageInfo{ texture.sampler, texture.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    write.pImageInfo = &imageInfo;

    data.device->updateDescriptorSets(writes);
    
    return reinterpret_cast<ImTextureID>(aDescriptorSet);
}

void ImGuiPlugin::bindDescriptorSet(VkCommandBuffer command_buffer, VkDescriptorSet aDescriptorSet) {
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    VkDescriptorSet desc_set[1] = { aDescriptorSet ? aDescriptorSet : descriptorSet };
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, desc_set, 0, nullptr);
}

ImTextureID ImGuiPlugin::addTexture(VulkanImageView& imageView) {
    auto aDescriptorSet = descriptorPool.allocate({ descriptorSetLayout}).front();
    descriptorSets.push_back(aDescriptorSet);

    auto writes = initializers::writeDescriptorSets<1>();
    auto& write = writes[0];
    write.dstSet = aDescriptorSet;
    write.dstBinding = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.descriptorCount = 1;
    VkDescriptorImageInfo imageInfo{ fontTexture.sampler, imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    write.pImageInfo = &imageInfo;

    data.device->updateDescriptorSets(writes);

    return reinterpret_cast<ImTextureID>(aDescriptorSet);
}


