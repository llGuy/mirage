#pragma once

#include "sdf.hpp"
#include "types.hpp"
#include "string.hpp"
#include "viewer.hpp"
#include "render_graph.hpp"

#include <vulkan/vulkan.h>

/* Allow for easy switching between the dumb SDF caster and rasterizer visualizers */
enum visualizer_type
{
  caster_visualizer,
  raster_visualizer
};

/* All renderer resources (textures, uniforms, pipelines, etc...) */
extern struct graphics_resources 
{
  /* Current viewer - perhaps some system will determine which agent to bind this to */
  viewer_desc viewer;
  float viewer_speed;

  /* All SDF information */
  sdf_info units_info;
  sdf_arrays units_arrays;
  sdf_debug units_debug;

  /* Visualizer type */
  visualizer_type vis_type;
} *ggfx;

/* Initialize all rendering systems - SDFs, post processing, etc... */
void init_core_render();

/* Run the entire rendering pipeline:
 * - Generate octree on GPU
 * - Rasterize cubes and perform SDF traces within the cubes
 * - Perform lighting
 * - Post processing */
void run_render();
