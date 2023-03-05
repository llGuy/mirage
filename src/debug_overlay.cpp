#include <vector>
#include <imgui.h>
#include <ImGuizmo.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include "pipeline.hpp"
#include "viewer.hpp"
#include "core_render.hpp"
#include "render_graph.hpp"
#include "debug_overlay.hpp"
#include "render_context.hpp"
#include "vulkan/vulkan_core.h"

struct debug_overlay_client {
    const char *name;
    bool open_by_default;
    debug_overlay_proc proc;
};

static std::vector<debug_overlay_client> clients_;
static bool has_focus_;

static pso dbg_rectangle_pso_;
static std::vector<dbg_rectangle> rectangles_;

static void imgui_callback_(VkResult result) {
    (void)result;
}

void init_debug_overlay() {
    GLFWwindow *window = gctx->window;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

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
    ImGui_ImplVulkan_Init(&init_info, gctx->swapchain_format);

    ImGui::StyleColorsDark();

    single_cmdbuf_generator single_generator;
    auto cmdbuf_info = single_generator.get_command_buffer();

    ImGui_ImplVulkan_CreateFontsTexture(cmdbuf_info.cmdbuf);

    single_generator.submit_command_buffer(cmdbuf_info, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);

    pso_config dbg_rect_config("dbg_rect.vert.spv", "dbg_rect.frag.spv");
    dbg_rect_config.add_color_attachment(
        gctx->swapchain_format, VK_BLEND_OP_ADD, 
        VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_DST_ALPHA);
    dbg_rect_config.configure_layouts(sizeof(dbg_rectangle));

    dbg_rectangle_pso_ = pso(dbg_rect_config);

}

void render_debug_overlay(VkCommandBuffer cmdbuf) {
    static bool draw_screen_boundary = true;

    // Render the rectangles
    if (draw_screen_boundary) {
        for (auto &rect : rectangles_) {
            dbg_rectangle_pso_.bind(cmdbuf);
            vkCmdPushConstants(
                    cmdbuf, dbg_rectangle_pso_.pso_layout(), VK_SHADER_STAGE_ALL,
                    0, sizeof(rect), &rect);

            vkCmdDraw(cmdbuf, 6, 1, 0, 0);
        }
    }

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();

    ImGuiIO &io = ImGui::GetIO();

    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

    // General stuff
    ImGui::Begin("Debug Overlay");

    static bool draw_grid = false;

    ImGui::Checkbox("Draw Grid", &draw_grid);
    ImGui::Checkbox("Draw Screen Boundary", &draw_screen_boundary);

    if (draw_grid) {
        viewer_desc &viewer = ggfx->viewer;
        glm::mat4 grid_transform = glm::mat4(1.0f);
        ImGuizmo::DrawGrid(&viewer.view[0][0], &viewer.projection[0][0], &grid_transform[0][0], 10.0f);
    }

    has_focus_ = ImGui::IsWindowFocused() || ImGuizmo::IsUsing();

    for (auto c : clients_) {
        ImGuiTreeNodeFlags flags = 0;
        if (c.open_by_default) {
            flags |= ImGuiTreeNodeFlags_DefaultOpen;
        }

        if (ImGui::TreeNodeEx(c.name, flags)) {
            c.proc();
            ImGui::TreePop();
        }
    }

    ImGui::End();

    ImGui::Render();

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdbuf);

    rectangles_.clear();
}

void register_debug_overlay_client(const char *name, debug_overlay_proc proc, bool open_by_default) {
    clients_.push_back({ .name = name, .proc = proc, .open_by_default = open_by_default });
}

bool overlay_has_focus() {
    return has_focus_;
}

void add_debug_rectangle(const dbg_rectangle &rect) {
    rectangles_.push_back(rect);
}
