#pragma once

#include "types.hpp"
#include "render_graph.hpp"
#include "debug_overlay.hpp"

// Supported voxel node sizes: 1, 2, 4, 8, 16, 32, 64, etc...
#define OCTREE_NODE_WIDTH 2
#define MAX_SDF_RENDER_INSTANCES 1024
// (roughly) Pixel width/height or each octree node
#define OCTREE_NODE_SCREEN_SIZE 128
#define OCTREE_NODE_SCALE (1.0f)

#define MAX_SDF_UNIT_COUNT 32

/* Shape types that the SDFs can take on (TODO: Add more shapes) */
enum sdf_type 
{
  sdf_sphere, sdf_cube, sdf_type_none
};

/* Determines how an SDF will affect the scene (whether it adds, subtracts,
 * does so smoothly, interects, etc...) */
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

/* For now, this is gonna hold all the data needed to describe an SDF */
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

/* Debug manipulator */
struct sdf_manipulator 
{
  m4x4 tx;
  u32 idx;
};

/* Gets sent to compute shader */
struct sdf_info 
{
  u32 unit_count;
  u32 add_unit_count;
  u32 sub_unit_count;
};

/* All the lists that SDF data requires */
struct sdf_arrays 
{
  sdf_unit *units;
  u32 *add_units;
  u32 *sub_units;
};

/* Debugging capabilities (for guizmos, etc...) */
struct sdf_debug 
{
  std::vector<sdf_manipulator> manipulators;
  s32 selected_manipulator;
  op_type selected_op;
  sdf_type selected_shape;
  ImGuizmo::OPERATION manip_op;
  int manip_op_idx;
};

using node_info = u32;

/* Allows easy extraction of information from the NODE_INFO unsigned int */
union octree_info_extracter 
{
  // To use this, set bits to the u32 and use the members below
  u32 bits;

  struct 
  {
    // Does this contain any information whatsoever
    u32 has_data : 1;

    // Is this a leaf (i.e., does this contain SDF information)
    // Otherwise, this is a node which points to another octree node
    u32 is_leaf : 1;

    // If is_leaf == 1, then this is a location into the SDF information pool
    // If is_leaf == 0, then this is a location into the octree node pool
    u32 loc : 30;
  };
};

/* Node in the SDF octree */
struct octree_node 
{
  // Nodes of the octree
  node_info child_nodes[OCTREE_NODE_WIDTH] /* Z */
                       [OCTREE_NODE_WIDTH] /* Y */
                       [OCTREE_NODE_WIDTH] /* X */;
};

constexpr uint32_t invalid_sdf_id = 0xFFFFFFFF;

/* An actual SDF list node containing SDF information at octree leaves */
struct sdf_list_node 
{
  union 
  {
    struct 
    {
      u32 count;
      u32 sdf_ids[14];
      u32 next;
    } first_elem;

    struct 
    {
      u32 sdf_ids[15];
      u32 next;
    } inner_elem;

    u32 elements[16];
  };
};

/* For now, just store this stuff - in future, will compact */
struct sdf_render_instance 
{
  v3 wposition;
  float dump0;

  u32 sdf_list_node_idx;
  u32 level; // Enforces the scale of the cube

  u32 dump1, dump2;
};

void init_sdf_units(render_graph &graph);
void update_sdf_units();
void init_sdf_octree(render_graph &graph, u32 max_sdf_units);
void clear_sdf_octree();
void add_sdf_unit(const sdf_unit &u, u32 u_id);
void update_sdf_octree_and_render(render_graph &graph);
void render_sdf(render_graph &graph);
