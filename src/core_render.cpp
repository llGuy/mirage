#include "imgui.h"
#include "sdf.hpp"
#include "file.hpp"
#include "time.hpp"
#include "viewer.hpp"
#include "memory.hpp"
#include "pipeline.hpp"
#include "core_render.hpp"
#include "render_graph.hpp"
#include "debug_overlay.hpp"
#include "render_context.hpp"
#include "vulkan/vulkan_core.h"
#include "glm/trigonometric.hpp"

graphics_resources *ggfx;

static render_graph *graph_;

void init_core_render() 
{
  ggfx = mem_alloc<graphics_resources>();
  graph_ = mem_alloc<render_graph>();

  graph_->setup_swapchain({
    .swapchain_image_count = gctx->images.size(),
    .images = gctx->images.data(),
    .image_views = gctx->image_views.data(),
    .extent = gctx->swapchain_extent
  });

  init_sdf_units(*graph_);
  init_debug_overlay();

  // Initialize default camera information
  ggfx->viewer.wposition = v3(3.2f, 4.0f, 7.7f);
  ggfx->viewer.wview_dir = glm::normalize(v3(-0.377f, -0.455f, -0.806f));
  ggfx->viewer.wup = v3(0.0f, 1.0f, 0.0f);
  ggfx->viewer.fov = 30.0f;
  ggfx->viewer_speed = 1.0f;

  // Register time uniform buffer
  graph_->register_buffer(RES("time-buffer"))
    .configure({ .size = sizeof(time_data) });

  // Register viewer uniform buffer
  graph_->register_buffer(RES("viewer-buffer"))
    .configure({ .size = sizeof(viewer_desc) });

  // Register debug overlay
  register_debug_overlay_client("Frame Information", 
    [] (){
      ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
      ImGui::SliderFloat("Max FPS", &gtime->max_fps, 30.0f, 120.0f);
  }, true);

  // Register viewer debug overlay
  register_debug_overlay_client("Viewer", 
    [] (){
      ImGui::Text("Position: %.1f %.1f %.1f", 
        ggfx->viewer.wposition.x, ggfx->viewer.wposition.y, ggfx->viewer.wposition.z);

      ImGui::Text("View Direction: %f %f %f", 
        ggfx->viewer.wview_dir.x, ggfx->viewer.wview_dir.y, ggfx->viewer.wview_dir.z);

      ImGui::SliderFloat("FOV", &ggfx->viewer.fov, 30.0f, 90.0f);

      ImGui::SliderFloat("Speed", &ggfx->viewer_speed, 0.0f, 15.0f);
  }, true);
}

void run_render() 
{
  // Starts a series of commands
  graph_->begin();

  // Update SDF stuff
  update_sdf_units(*graph_);

  // Update time data
  time_data tdata = { gtime->frame_dt, gtime->current_time };
  graph_->add_buffer_update(RES("time-buffer"), &tdata);

  // Update viewer data
  ggfx->viewer.tick({ gctx->swapchain_extent.width, gctx->swapchain_extent.height });
  graph_->add_buffer_update(RES("viewer-buffer"), &ggfx->viewer);

  { // Cast rays to SDF units
    auto &pass = graph_->add_compute_pass(STG("sdf-cast-pass"));
    pass.set_source("sdf_cast");
    pass.add_storage_image(RES("sdf-cast-target"));
    pass.add_uniform_buffer(RES("time-buffer"));
    pass.add_uniform_buffer(RES("sdf-info-buffer"));
    pass.add_uniform_buffer(RES("sdf-units-buffer"));
    pass.add_uniform_buffer(RES("sdf-add-buffer"));
    pass.add_uniform_buffer(RES("sdf-sub-buffer"));
    pass.add_uniform_buffer(RES("viewer-buffer"));
    pass.dispatch_waves(16, 16, 1, RES("sdf-cast-target"));
  }

  { // Render debug overlay information
    auto &pass = graph_->add_render_pass(STG("debug-overlay"));
    pass.add_color_attachment(RES("sdf-cast-target"));
    pass.draw_commands(
        [] (render_pass::draw_package package) {
        auto tracker = package.graph->get_resource_tracker();
        render_debug_overlay(package.cmdbuf, tracker);
        }, nullptr);
  }

  // Present to the screen
  graph_->present(RES("sdf-cast-target"));

  // Finishes a series of commands
  graph_->end();
}
