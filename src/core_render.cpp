#include "time.hpp"
#include "memory.hpp"
#include "core_render.hpp"
#include "render_context.hpp"

graphics_resources *ggfx;

void init_core_render() {
    ggfx = mem_alloc<graphics_resources>();

    ggfx->graph.setup_swapchain({
        .swapchain_image_count = gctx->images.size(),
        .images = gctx->images.data(),
        .image_views = gctx->image_views.data(),
        .extent = gctx->swapchain_extent
    });
}

void run_render() {
    poll_input();

    render_graph *graph = &ggfx->graph;

    // Starts a series of commands
    graph->begin();

    { // Example setup a compute pass (all in immediate)
        compute_pass &pass = graph->add_compute_pass(STG("compute-example"));
        pass.set_source("basic"); // Name of the file (can use custom filename resolution
        pass.send_data(gtime->current_time); // Can send any arbitrary data
        pass.add_storage_image(RES("compute-target")); // Sets the render target
        pass.dispatch_waves(16, 16, 1, RES("compute-target"));
    }

    // Present to the screen
    graph->present(RES("compute-target"));

    // Finishes a series of commands
    graph->end();
}
