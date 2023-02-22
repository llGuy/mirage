#include <vector>
#include <imgui.h>
#include <ImGuizmo.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include "viewer.hpp"
#include "render_graph.hpp"
#include "debug_overlay.hpp"
#include "render_context.hpp"

struct debug_overlay_client {
    const char *name;
    bool open_by_default;
    debug_overlay_proc proc;
};

static std::vector<debug_overlay_client> clients_;
static bool has_focus_;

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
}

void edit_transform(const viewer_desc &viewer, glm::mat4 &tx, ImGuizmo::OPERATION op) {
    ImGuizmo::MODE mode = ImGuizmo::WORLD;

    ImGuiIO &io = ImGui::GetIO();

    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
    ImGuizmo::Manipulate(&viewer.view[0][0], &viewer.projection[0][0], op, mode, &tx[0][0]);
}

#if 0
void EditTransform()
{
    static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::ROTATE);
    static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::WORLD);
    if (ImGui::IsKeyPressed(90))
        mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
    if (ImGui::IsKeyPressed(69))
        mCurrentGizmoOperation = ImGuizmo::ROTATE;
    if (ImGui::IsKeyPressed(82)) // r Key
        mCurrentGizmoOperation = ImGuizmo::SCALE;
    if (ImGui::RadioButton("Translate", mCurrentGizmoOperation == ImGuizmo::TRANSLATE))
        mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
    ImGui::SameLine();
    if (ImGui::RadioButton("Rotate", mCurrentGizmoOperation == ImGuizmo::ROTATE))
        mCurrentGizmoOperation = ImGuizmo::ROTATE;
    ImGui::SameLine();
    if (ImGui::RadioButton("Scale", mCurrentGizmoOperation == ImGuizmo::SCALE))
        mCurrentGizmoOperation = ImGuizmo::SCALE;
    float matrixTranslation[3], matrixRotation[3], matrixScale[3];
    ImGuizmo::DecomposeMatrixToComponents(matrix.m16, matrixTranslation, matrixRotation, matrixScale);
    ImGui::InputFloat3("Tr", matrixTranslation, 3);
    ImGui::InputFloat3("Rt", matrixRotation, 3);
    ImGui::InputFloat3("Sc", matrixScale, 3);
    ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, matrix.m16);

    if (mCurrentGizmoOperation != ImGuizmo::SCALE)
    {
        if (ImGui::RadioButton("Local", mCurrentGizmoMode == ImGuizmo::LOCAL))
            mCurrentGizmoMode = ImGuizmo::LOCAL;
        ImGui::SameLine();
        if (ImGui::RadioButton("World", mCurrentGizmoMode == ImGuizmo::WORLD))
            mCurrentGizmoMode = ImGuizmo::WORLD;
    }
    static bool useSnap(false);
    if (ImGui::IsKeyPressed(83))
        useSnap = !useSnap;
    ImGui::Checkbox("", &useSnap);
    ImGui::SameLine();
    vec_t snap;
    switch (mCurrentGizmoOperation)
    {
        case ImGuizmo::TRANSLATE:
            snap = config.mSnapTranslation;
            ImGui::InputFloat3("Snap", &snap.x);
            break;
        case ImGuizmo::ROTATE:
            snap = config.mSnapRotation;
            ImGui::InputFloat("Angle Snap", &snap.x);
            break;
        case ImGuizmo::SCALE:
            snap = config.mSnapScale;
            ImGui::InputFloat("Scale Snap", &snap.x);
            break;
    }
    ImGuiIO& io = ImGui::GetIO();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
    ImGuizmo::Manipulate(camera.mView.m16, camera.mProjection.m16, mCurrentGizmoOperation, mCurrentGizmoMode, matrix.m16, NULL, useSnap ? &snap.x : NULL);
}
#endif

void render_debug_overlay(VkCommandBuffer cmdbuf) {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // General stuff
    ImGui::Begin("Debug Overlay");

    has_focus_ = ImGui::IsWindowFocused();

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
}

void register_debug_overlay_client(const char *name, debug_overlay_proc proc, bool open_by_default) {
    clients_.push_back({ .name = name, .proc = proc, .open_by_default = open_by_default });
}

bool overlay_has_focus() {
    return has_focus_;
}
