#include "file.hpp"
#include "glm/trigonometric.hpp"
#include "pipeline.hpp"
#include "render_graph.hpp"
#include "time.hpp"
#include "memory.hpp"
#include "core_render.hpp"
#include "render_context.hpp"
#include "vulkan/vulkan_core.h"

graphics_resources *ggfx;
pso triangle_pso;

void init_core_render() {
    ggfx = mem_alloc<graphics_resources>();

    ggfx->graph.setup_swapchain({
        .swapchain_image_count = gctx->images.size(),
        .images = gctx->images.data(),
        .image_views = gctx->image_views.data(),
        .extent = gctx->swapchain_extent
    });

    { // Create triangle shader
        pso_config triangle_pso_config("triangle.vert.spv", "triangle.frag.spv");
        triangle_pso_config.add_color_attachment(gctx->swapchain_format);
        triangle_pso = pso(triangle_pso_config);
    }

    { // Register example uniform buffer
        auto &ubo = ggfx->graph.register_buffer(RES("example-ubo"));
        ubo.configure({ .size = sizeof(float) });
    }
}

void run_render() {
    poll_input();

    render_graph *graph = &ggfx->graph;

    // Starts a series of commands
    graph->begin();

    { // Update a UBO
        float payload = glm::sin(gtime->current_time);
        graph->add_buffer_update(RES("example-ubo"), &payload, 0, sizeof(float));
    }

    { // Example setup a compute pass (all in immediate)
        compute_pass &pass = graph->add_compute_pass(STG("compute-example"));
        pass.set_source("basic");
        pass.send_data(gtime->current_time);

        // Set target
        image_info target_info = { .extent = { gctx->swapchain_extent.width/2, gctx->swapchain_extent.height/2, 1 } };
        pass.add_storage_image(RES("compute-target"), target_info);

        pass.add_uniform_buffer(RES("example-ubo"));
        pass.dispatch_waves(16, 16, 1, RES("compute-target"));
    }

    { // Example of a render pass
        render_pass &pass = graph->add_render_pass(STG("raster-example"));
        pass.add_color_attachment(RES("compute-target")); // Don't clear target
        pass.draw_commands([] (VkCommandBuffer cmdbuf, VkRect2D rect, void *data) {
            pso &triangle_pso = *(pso *)data;
            triangle_pso.bind(cmdbuf);
            vkCmdDraw(cmdbuf, 3, 1, 0, 0);
        }, triangle_pso);
    }

    graph->add_image_blit(RES("compute-target"), RES("backbuffer"));

    // Present to the screen
    graph->present(RES("backbuffer"));

    // Finishes a series of commands
    graph->end();
}
