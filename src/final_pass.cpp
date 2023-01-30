#include "compute.hpp"
#include "core_render.hpp"

static compute_pass final_pass_;

void init_final_pass() {
    final_pass_ = make_compute_pass<no_push_constant>(
        "blob_cast",
        uprototype{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE },
        uprototype{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER },
        uprototype{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER }
    );
}

void run_final_pass(render_graph &graph, texture &target) {
    final_pass_.bind_resources<no_push_constant>(graph, nullptr,
        target, ggfx->time_uniform_data, ggfx->blob_data);

    final_pass_.run(graph, gctx->swapchain_extent.width / 16, gctx->swapchain_extent.height / 16, 1);
}
