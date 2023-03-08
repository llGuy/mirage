#pragma once

#include "sdf.hpp"
#include "types.hpp"
#include "string.hpp"
#include "viewer.hpp"
#include "render_graph.hpp"

#include <vulkan/vulkan.h>

// All renderer resources (textures, uniforms, pipelines, etc...)
extern struct graphics_resources 
{
  // Current viewer - perhaps some system will determine which agent to bind this to
  viewer_desc viewer;
  float viewer_speed;

  // All SDF information
  sdf_info units_info;
  sdf_arrays units_arrays;
  sdf_debug units_debug;
} *ggfx;

void init_core_render();
void run_render();

// To use whenever referring to a string that relates to a rendering resource
extern uint32_t gres_name_id_counter;
#define rdg(x) (uid_string{     \
  x, sizeof(x),               \
  get_id<crc32<sizeof(x) - 2>(x) ^ 0xFFFFFFFF>(gres_name_id_counter)})
