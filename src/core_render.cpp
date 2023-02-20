#include "file.hpp"
#include "glm/trigonometric.hpp"
#include "pipeline.hpp"
#include "render_graph.hpp"
#include "sdf.hpp"
#include "time.hpp"
#include "memory.hpp"
#include "core_render.hpp"
#include "render_context.hpp"
#include "vulkan/vulkan_core.h"

graphics_resources *ggfx;

static render_graph *graph_;

void init_core_render() {
    ggfx = mem_alloc<graphics_resources>();
    graph_ = mem_alloc<render_graph>();

    graph_->setup_swapchain({
        .swapchain_image_count = gctx->images.size(),
        .images = gctx->images.data(),
        .image_views = gctx->image_views.data(),
        .extent = gctx->swapchain_extent
    });

    init_sdf_units(*graph_);

    // Register time uniform buffer
    graph_->register_buffer(RES("time-buffer"))
        .configure({ .size = sizeof(time_data) });
}

void run_render() {
    poll_input();

    // Starts a series of commands
    graph_->begin();

    // Update SDF stuff
    update_sdf_units(*graph_);

    // Update time data
    time_data tdata = { gtime->frame_dt, gtime->current_time };
    graph_->add_buffer_update(RES("time-buffer"), &tdata);

    { // Cast rays to SDF units
        auto &pass = graph_->add_compute_pass(STG("sdf-cast-pass"));
        pass.set_source("sdf_cast");
        pass.add_storage_image(RES("sdf-cast-target"));
        pass.add_uniform_buffer(RES("time-buffer"));
        pass.add_uniform_buffer(RES("sdf-units-buffer"));
        pass.dispatch_waves(16, 16, 1, RES("sdf-cast-target"));
    }

    // Present to the screen
    graph_->present(RES("sdf-cast-target"));

    // Finishes a series of commands
    graph_->end();
}
