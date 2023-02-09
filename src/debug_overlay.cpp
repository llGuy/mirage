#include <imgui.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include "render_graph.hpp"
#include "debug_overlay.hpp"
#include "render_context.hpp"

#if 0
static void imgui_callback_(VkResult result) {
    (void)result;
}

void init_debug_overlay() {
    GLFWwindow *window = gctx->window;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    // We won't clear the contents of the framebuffer
    VkAttachmentDescription attachment = {};
    attachment.format = gctx->swapchain_format;
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment = {};
    color_attachment.attachment = 0;
    color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info.attachmentCount = 1;
    info.pAttachments = &attachment;
    info.subpassCount = 1;
    info.pSubpasses = &subpass;
    info.dependencyCount = 1;
    info.pDependencies = &dependency;

    VK_CHECK(vkCreateRenderPass(gctx->device, &info, nullptr, &gctx->imgui_render_pass));

    ImGui_ImplGlfw_InitForVulkan(window, true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = gctx->instance;
    init_info.PhysicalDevice = gctx->gpu;
    init_info.Device = gctx->device;
    init_info.QueueFamily = gctx->graphics_family;
    init_info.Queue = gctx->graphics_queue;
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = gctx->descriptor_pool;
    init_info.Allocator = NULL;
    init_info.MinImageCount = gctx->images.size();
    init_info.ImageCount = gctx->images.size();
    init_info.CheckVkResultFn = &imgui_callback_;
    ImGui_ImplVulkan_Init(&init_info, gctx->imgui_render_pass);

    ImGui::StyleColorsDark();

    VkCommandBuffer command_buffer;
    VkCommandBufferAllocateInfo command_buffer_info = {};
    command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_info.commandBufferCount = 1;
    command_buffer_info.commandPool = gctx->command_pool;
    command_buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    vkAllocateCommandBuffers(gctx->device, &command_buffer_info, &command_buffer);

    render_graph graph (command_buffer, render_graph::one_time);

    ImGui_ImplVulkan_CreateFontsTexture(command_buffer);

    graph.submit(gctx->graphics_queue, VK_NULL_HANDLE, VK_NULL_HANDLE,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_NULL_HANDLE);
}

void render_debug_overlay(render_graph &graph) {
    // Start a render pass


    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // General stuff
    ImGui::Begin("General");
    ImGui::Text("Framerate: %.1f", ImGui::GetIO().Framerate);

#if 0
    for (uint32_t i = 0; i < g_ctx->debug.ui_proc_count; ++i) {
        (g_ctx->debug.ui_procs[i])();
    }
#endif

    ImGui::End();

    ImGui::Render();

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), graph.cmdbuf());
}
#endif
