#pragma once

#include "types.hpp"
#include "render_graph.hpp"
#include "debug_overlay.hpp"

constexpr u32 max_sdf_unit_count = 32;

enum sdf_type 
{
  sdf_sphere, sdf_cube, sdf_type_none
};

enum op_type 
{
  sdf_add, 
  sdf_sub,
  sdf_intersect,
  sdf_smooth_add,
  sdf_smooth_sub,
  sdf_smooth_intersect,
  op_type_none
};

struct sdf_unit 
{
  // Add rotations later
  v4 position;
  v4 scale;
  u32 type;
  u32 op;

  u32 manipulator;
  u32 pad;
};

struct sdf_manipulator 
{
  m4x4 tx;
  u32 idx;
};

// Gets sent to compute shader
struct sdf_info 
{
  u32 unit_count;

  // One count for each type of sdf unit
  u32 add_unit_count;
  u32 sub_unit_count;
};

struct sdf_arrays 
{
  sdf_unit *units;
  u32 *add_units;
  u32 *sub_units;
};

struct sdf_debug 
{
  std::vector<sdf_manipulator> manipulators;
  s32 selected_manipulator;
  op_type selected_op;
  sdf_type selected_shape;
  ImGuizmo::OPERATION manip_op;
  int manip_op_idx;
};

void init_sdf_units(render_graph &graph);
void update_sdf_units();
void init_sdf_octree(render_graph &graph);
void clear_sdf_octree();
void add_sdf_unit(const sdf_unit &u, u32 u_id);
void update_sdf_octree();
void render_sdf(render_graph &graph);
