/* This is the CPU backend of the octree - provides octree subdivision,
 * SDF list node creation - and feeds all this data to storage buffers
 * for rendering */

#include <imgui.h>
#include <ImGuizmo.h>
#include "arena.hpp"
#include "core_render.hpp"
#include "debug_overlay.hpp"
#include "heap_array.hpp"
#include "math_util.hpp"
#include "memory.hpp"
#include "bits.hpp"
#include "pipeline.hpp"
#include "render_context.hpp"
#include "sdf.hpp"
#include "viewer.hpp"
#include "render_graph.hpp"

// Pool of octree nodes
static arena_pool<octree_node> octree_nodes_;

// Pool of SDF leaf nodes
static arena_pool<sdf_list_node> sdf_list_nodes_;

// List of SDF cube nodes
static heap_array<sdf_render_instance> sdf_render_instances_;
static uint32_t sdf_render_instance_count_;

// Max level of the octree
static float octree_max_level_;

// Root node
static node_info root_;

// Rendering pipeline state object for the cubes
static pso sdf_raster_pso_;

struct sdf_octree_debug 
{
  v3 cube_center;
  v3 cube_scale;

  float node_cube_width;
  float lod_level;
  float screen_space_level;
  float final_level;

  v3 cube_start;
  v3 cube_end;
  v3 count;
} debug_;

static v2 pos_to_pixel_coords_(v2 ndc)
{
  v2 extent = v2((float)gctx->swapchain_extent.width, (float)gctx->swapchain_extent.height);
  v2 uv = (ndc + v2(1.0f)) / 2.0f;

  return uv * extent;
}

static v2 dir_to_pixel_coords_(v2 ndc)
{
  v2 extent = v2((float)gctx->swapchain_extent.width, (float)gctx->swapchain_extent.height);
  v2 uv = ndc / 2.0f;

  return uv * extent;
}

bool node_has_data(node_info n) 
{
  octree_info_extracter *e = (octree_info_extracter *)&n;
  return e->has_data;
}

bool is_node_leaf(node_info n) 
{
  octree_info_extracter *e = (octree_info_extracter *)&n;
  return e->is_leaf;
}

u32 node_loc(node_info n) 
{
  octree_info_extracter *e = (octree_info_extracter *)&n;
  return e->loc;
}

void set_node_has_data(node_info n, bool value) 
{
  octree_info_extracter *e = (octree_info_extracter *)&n;
  e->has_data = value;
}

void set_is_node_leaf(node_info n, bool value) 
{
  octree_info_extracter *e = (octree_info_extracter *)&n;
  e->is_leaf = value;
}

void set_node_loc(node_info n, u32 loc) 
{
  octree_info_extracter *e = (octree_info_extracter *)&n;
  e->loc = loc;
}

// If loc actually refers to a octree node, HAS_DATA has to be true
node_info create_node_info(bool has_data, bool is_leaf, u32 loc) 
{
  octree_info_extracter e;
  e.has_data = has_data;
  e.is_leaf = is_leaf;
  e.loc = loc;

  return e.bits;
}

static void sdf_octree_gen_debug_proc_() 
{
  viewer_desc &viewer = ggfx->viewer;

  float cube_width = OCTREE_NODE_SCALE * glm::pow(2.0f, debug_.final_level);
  ImGui::Text("World Cube Width: %f", cube_width);
  ImGui::Text("World Cube Level: %f", debug_.final_level);
  ImGui::Text("Screen Space Level: %f", debug_.screen_space_level);
  ImGui::Text("LOD Level: %f", debug_.lod_level);
  ImGui::Text("Count: %f %f %f", debug_.count.x, debug_.count.y, debug_.count.z);
  ImGui::Text("Cube Start: %f %f %f", debug_.cube_start.x, debug_.cube_start.y, debug_.cube_start.z);
  ImGui::Text("Cube End: %f %f %f", debug_.cube_end.x, debug_.cube_end.y, debug_.cube_end.z);
  ImGui::Text("SDF Render Instance Count: %d", sdf_render_instance_count_);

  ImGui::Separator();

  // Display render instance information
  if (ImGui::TreeNode("List Node Info"))
  {
    for (int i = 0; i < sdf_list_nodes_.size(); ++i)
    {
      sdf_list_node *node = sdf_list_nodes_.get_node(i);
      ImGui::BulletText("Node %d has %d SDFs", i, node->first_elem.count);
    }

    ImGui::TreePop();
  }
}

// Get level if we only considered the LOD (distance from camera)
static float find_sdf_unit_level_lod_(const sdf_unit &u) 
{
  viewer_desc &viewer = ggfx->viewer;

  float average_view_z = (viewer.view * v4(v3(u.position), 1.0f)).z;

  // What the fuckk do I do here
  // Find out world space length of the boxes that will enclose the shape
  m4x4 &p_tx = viewer.projection;

  // Width of the node in screen space
  float ss_node_width = ((float)OCTREE_NODE_SCREEN_SIZE / (float)gctx->swapchain_extent.width) * 2.0f;
  float ws_node_width = ss_node_width * (p_tx[3][2] * average_view_z) / p_tx[0][0];
  float n = glm::max(0.0f, glm::ceil(glm::log2(ws_node_width / OCTREE_NODE_SCALE)));

  return n;
}

// Get level if we only considered the screen space size of the object
static float find_sdf_unit_level_screen_size_(const sdf_unit &u) 
{
  // First find the screen space width of the encapsulating cube
  viewer_desc &viewer = ggfx->viewer;
  float average_view_z = (viewer.view * v4(v3(u.position), 1.0f)).z;

  v3 scale = v3(u.scale) + v3(0.1f);
  v3 low = v3(u.position) - scale;

  v3 bound_corners[] = 
  {
    low,
    low + 2.0f*v3(scale.x, 0.0f, 0.0f),
    low + 2.0f*v3(0.0f, scale.y, 0.0f),
    low + 2.0f*v3(0.0f, 0.0f, scale.z),

    low + 2.0f*v3(scale.x, scale.y, 0.0f),
    low + 2.0f*v3(scale.x, 0.0f, scale.z),
    low + 2.0f*v3(0.0f, scale.y, scale.z),
    low + 2.0f*v3(scale.x, scale.y, scale.z)
  };

  v2 ss_low = v2(FLT_MAX);
  v2 ss_high = v2(-FLT_MAX);

  for (int i = 0; i < 8; ++i) 
  {
    v4 clip = viewer.view_projection * v4(bound_corners[i], 1.0f);

    v2 screen_space_coord = v2(clip) / clip.w;

    ss_low = glm::min(screen_space_coord, ss_low);
    ss_high = glm::max(screen_space_coord, ss_high);
  }

  v2 rng = v2(ss_high.x - ss_low.x, ss_high.y - ss_low.y);
  v2 px_rng = dir_to_pixel_coords_(rng);

  float level = 0.0f;

  // Figure out which range is smaller, this will dictacte the screen space width of cube
  if (px_rng.x < px_rng.y) 
  {
    // Use X component for cube width
    m4x4 &p_tx = viewer.projection;
    float ss_node_width = rng.x;
    float ws_node_width = ss_node_width * (p_tx[3][2] * average_view_z) / p_tx[0][0];
    level = glm::max(0.0f, glm::ceil(glm::log2(ws_node_width / OCTREE_NODE_SCALE)));
  }
  else 
  {
    // Use Y componentf or cube width
    m4x4 &p_tx = viewer.projection;
    float ss_node_width = rng.y;
    float ws_node_width = ss_node_width * (p_tx[3][2] * average_view_z) / p_tx[1][1];
    level = glm::max(0.0f, glm::ceil(glm::log2(ws_node_width / OCTREE_NODE_SCALE)));
  }

  return level;
}

float find_sdf_unit_level(const sdf_unit &u) 
{
  float level_lod = find_sdf_unit_level_lod_(u);
  float level_screen_space = find_sdf_unit_level_screen_size_(u);
  return glm::min(level_screen_space, level_lod);
}

// More optimized for inserting multiple nodes which are close to eachother
void insert_batched_sdf_nodes_into_octree() 
{

}

// For now just write ugly code - will clean up in the future when this works
// and we do GPU port of all this shit
void push_back_sdf_list_node(sdf_list_node *node, u32 value) 
{
  // Calculate how many nodes themselves we need to traverse
  int elem_count = (int)node->first_elem.count;

  float to_traverse_minus_one = glm::floor(((float)elem_count - 14.0f) / 15.0f);
  int to_traverse = 1 + (int)to_traverse_minus_one;

  int elems_in_node = (to_traverse == 0) ? elem_count : (elem_count-14)%15;
  int max_elems_in_node = (to_traverse == 0) ? 14 : 15;

  sdf_list_node *current_node = node;

  // Traverse through the tree
  if (to_traverse)
    current_node = sdf_list_nodes_.get_node(current_node->first_elem.next);
  for (int i = 0; i < to_traverse-1; ++i)
    current_node = sdf_list_nodes_.get_node(current_node->inner_elem.next);

  if (elems_in_node == max_elems_in_node) 
  {
    // Create a new node and push back to that node
    u32 list_node = sdf_list_nodes_.get_node_idx(sdf_list_nodes_.alloc());

    if (to_traverse == 0)
      current_node->first_elem.next = list_node;
    else
      current_node->inner_elem.next = list_node;

    current_node = sdf_list_nodes_.get_node(list_node);
    current_node->inner_elem.sdf_ids[0] = value;
  }
  else 
  {
    if (to_traverse == 0) 
    {
      current_node->first_elem.sdf_ids[elems_in_node] = value;
    }
    else 
    {
      current_node->inner_elem.sdf_ids[elems_in_node] = value;
    }
  }

  node->first_elem.count++;
}

// The position is in octree space (where 0,0,0 is at the octree origin)
void insert_sdf_node_into_octree(float level, v3 pos, u32 value) 
{
  int dst_level = (int)level;
  float scale = glm::pow(2.0f, level);

  node_info *current_node = &root_;
  v3 center = v3(0.0f);
  bool found = false;

  int final_level = (int)octree_max_level_;
  for (; final_level >= 0; --final_level) 
  {
    if (dst_level == final_level || is_node_leaf(*current_node)) 
    {
      found = true;
      break;
    }

    // If the node is empty (i.e. doesn't point to anything), then, create a new one
    if (!node_has_data(*current_node)) 
    {
      // Create a new node
      node_info new_node = create_node_info(
          true, false, octree_nodes_.get_node_idx(octree_nodes_.alloc()));
      *current_node = new_node;
    }

    // Current subdivision level
    float current_level = (float)final_level;
    float node_size = glm::pow(2.0f, current_level);
    float child_node_size = node_size / 2.0f;

    // Figure out which octant the position is in
    v3 level_start_pos = center - v3(child_node_size);
    v3 offset = pos - level_start_pos;

    assert(offset.x >= 0.0f && offset.y >= 0.0f && offset.z >= 0.0f &&
      offset.x < node_size && offset.y < node_size && offset.z < node_size);

    offset /= child_node_size;

    // This is the index into the octree node 3D array
    iv3 child_idx = iv3(glm::floor(offset));

    assert(child_idx.x >= 0 && child_idx.y >= 0 && child_idx.z >= 0 &&
      child_idx.x < OCTREE_NODE_WIDTH && child_idx.y < OCTREE_NODE_WIDTH && child_idx.z < OCTREE_NODE_WIDTH);

    center = level_start_pos + v3(child_idx) * child_node_size + v3(child_node_size/2.0f);

    octree_node *node_ptr = octree_nodes_.get_node(node_loc(*current_node));
    current_node = &node_ptr->child_nodes[child_idx.z][child_idx.y][child_idx.x];
  }

  assert(found);

  if (!is_node_leaf(*current_node)) 
  {
    // This means that this node wasn't created yet - must create (must be an empty node as of now)
    if (!node_has_data(*current_node)) 
    {
      // Node has data and is a leaf
      node_info new_node = create_node_info(
          true, true, sdf_list_nodes_.get_node_idx(sdf_list_nodes_.alloc()));
      *current_node = new_node;

      float node_size = glm::pow(2.0f, final_level);
      float child_node_size = node_size / 2.0f;

      v3 start_pos = center - v3(child_node_size);

      // Every time we create a new leaf node, we need to push it to the render instances array
      sdf_render_instance inst = 
      {
        .level = (u32)final_level,
        .sdf_list_node_idx = node_loc(*current_node),
        .wposition = start_pos,
        .dump0 = -42.0f
      };

      sdf_render_instances_[sdf_render_instance_count_++] = inst;
    }
  }

  // Now that that the node is definitely created and valid, we can proceed wth adding
  // the SDF information to that node
  sdf_list_node *node = sdf_list_nodes_.get_node(node_loc(*current_node));

  push_back_sdf_list_node(node, value);
}

// Add an SDF unit to the octree
void add_sdf_unit(const sdf_unit &u, u32 u_id)
{
  v3 scale = v3(u.scale) + v3(0.1f);
  v3 low = v3(u.position) - scale;
  v3 high = v3(u.position) + scale;

  float level_lod = find_sdf_unit_level_lod_(u);
  float level_screen_space = find_sdf_unit_level_screen_size_(u);
  float level = glm::min(level_screen_space, level_lod);

  // Width of the cubes which will encompass this SDF unit
  float cube_width = OCTREE_NODE_SCALE * glm::pow(2.0f, level);

  v3 cube_start = floor_to(low, cube_width);
  v3 cube_end = ceil_to(high, cube_width);
  v3 rng = cube_end - cube_start;
  v3 count = rng / cube_width;

  apply_3d(count, [&](const iv3 &off)
  {
    v3 pos = cube_start + v3(off) * cube_width;
    insert_sdf_node_into_octree(level, pos, u_id);
  });

  // Just for debugging purposes
  if (ggfx->units_debug.selected_manipulator == u.manipulator) 
  {
    debug_.cube_center = v3(u.position);
    debug_.cube_scale = 2.0f*scale;
    debug_.lod_level = level_lod;
    debug_.screen_space_level = level_screen_space;
    debug_.final_level = level;
    debug_.cube_start = cube_start;
    debug_.cube_end = cube_end;
    debug_.count = count;
  }
}

static void add_cube_outline_(v3 low, v3 high, bool is_leaf) 
{
  v4 color = is_leaf ? v4(0.0f, 1.0f, 0.0f, 1.0f) : v4(1.0f, 0.0f, 0.0f, 0.2f);

  v3 rng = high - low;

  v4 cube_positions[8] = 
  {
    v4(0.0f, 0.0f, 0.0f, 1.0f),
    v4(0.0f, 0.0f, 1.0f, 1.0f),
    v4(0.0f, 1.0f, 0.0f, 1.0f),
    v4(0.0f, 1.0f, 1.0f, 1.0f),
    v4(1.0f, 0.0f, 0.0f, 1.0f),
    v4(1.0f, 0.0f, 1.0f, 1.0f),
    v4(1.0f, 1.0f, 0.0f, 1.0f),
    v4(1.0f, 1.0f, 1.0f, 1.0f),
  };

  for (int i = 0; i < 8; ++i)
    cube_positions[i] = v4(low, 0.0f) + cube_positions[i] * v4(rng, 1.0f);

  dbg_line line = { .color = color };

  line.positions[0] = cube_positions[0];
  line.positions[1] = cube_positions[4];
  add_debug_line(line);
  line.positions[0] = cube_positions[1];
  line.positions[1] = cube_positions[5];
  add_debug_line(line);
  line.positions[0] = cube_positions[2];
  line.positions[1] = cube_positions[6];
  add_debug_line(line);
  line.positions[0] = cube_positions[3];
  line.positions[1] = cube_positions[7];
  add_debug_line(line);

  line.positions[0] = cube_positions[0];
  line.positions[1] = cube_positions[1];
  add_debug_line(line);
  line.positions[0] = cube_positions[0];
  line.positions[1] = cube_positions[2];
  add_debug_line(line);
  line.positions[0] = cube_positions[1];
  line.positions[1] = cube_positions[3];
  add_debug_line(line);
  line.positions[0] = cube_positions[2];
  line.positions[1] = cube_positions[3];
  add_debug_line(line);

  line.positions[0] = cube_positions[4];
  line.positions[1] = cube_positions[5];
  add_debug_line(line);
  line.positions[0] = cube_positions[4];
  line.positions[1] = cube_positions[6];
  add_debug_line(line);
  line.positions[0] = cube_positions[5];
  line.positions[1] = cube_positions[7];
  add_debug_line(line);
  line.positions[0] = cube_positions[6];
  line.positions[1] = cube_positions[7];
  add_debug_line(line);
}

static void dbg_add_octree_lines_(node_info *node, v3 center, float level) 
{
  // Just render this node's squares
  float node_size = glm::pow(2.0f, level);
  float child_node_size = node_size / 2.0f;

  v3 level_start_pos = center - v3(child_node_size);
  v3 level_end_pos = center + v3(child_node_size);

  add_cube_outline_(level_start_pos, level_end_pos, is_node_leaf(*node));

  if (node_has_data(*node) && !is_node_leaf(*node)) 
  {
    octree_node *node_ptr = octree_nodes_.get_node(node_loc(*node));

    apply_3d(iv3(OCTREE_NODE_WIDTH), [&](const iv3 &off)
    {
      v3 new_center = level_start_pos + 
        v3(off) * child_node_size + v3(child_node_size/2.0f);

      node_info *child_node = &node_ptr->child_nodes[off.z][off.y][off.x];

      dbg_add_octree_lines_(child_node, new_center, level-1.0f);
    });
  }
}

// Also register the buffers that will be used
void init_sdf_octree(render_graph &graph, u32 max_sdf_units)
{
  // Initialize octree information
  octree_max_level_ = 8.0f;
  octree_nodes_ = arena_pool<octree_node>(1024);
  root_ = create_node_info(true, false, octree_nodes_.get_node_idx(octree_nodes_.alloc()));

  // Initialize SDF list nodes (containing SDF information at octree leaves)
  sdf_list_nodes_ = arena_pool<sdf_list_node>(1024);
  sdf_render_instances_ = heap_array<sdf_render_instance>(MAX_SDF_RENDER_INSTANCES);
  
  // Rendering - SDF Render Instances, List Nodes, and SDF Unit data
  graph.register_buffer(RES("sdf-instances"))
    .configure({ .size = sizeof(sdf_render_instance) * MAX_SDF_RENDER_INSTANCES, binding::type::storage_buffer });

  graph.register_buffer(RES("sdf-units")) // This should become storage buffer
    .configure({ .size = (u32)sizeof(sdf_unit) *  max_sdf_units, binding::type::uniform_buffer });

  graph.register_buffer(RES("sdf-list-nodes"))
    .configure({ .size = (u32)sizeof(sdf_list_node) * 1024, binding::type::storage_buffer });

  graph.register_buffer(RES("sdf-add-buffer"))
    .configure({ .size = max_sdf_units * (u32)sizeof(u32) });

  graph.register_buffer(RES("sdf-sub-buffer"))
    .configure({ .size = max_sdf_units * (u32)sizeof(u32) });

  graph.register_buffer(RES("sdf-info-buffer"))
    .configure({ .size = sizeof(sdf_info) });

  // Create rendering pipeline
  pso_config cube_raster_config("voxel_node.vert.spv", "voxel_node.frag.spv");
  cube_raster_config.add_color_attachment(gctx->swapchain_format);
  cube_raster_config.add_depth_attachment(gctx->depth_format);
  cube_raster_config.configure_layouts(0,
    pso_descriptor{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 },
    pso_descriptor{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 },
    pso_descriptor{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
    pso_descriptor{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 });

  sdf_raster_pso_ = pso(cube_raster_config);

  // Debugging
  register_debug_overlay_client("SDF Octree Gen", sdf_octree_gen_debug_proc_, true);
}

// Clears all the arena allocators and reinitializes the root node
void clear_sdf_octree()
{
  octree_nodes_.clear();
  sdf_list_nodes_.clear();

  root_ = create_node_info(true, false, octree_nodes_.get_node_idx(octree_nodes_.alloc()));
  sdf_render_instance_count_ = 0;
}

void update_sdf_octree_and_render(render_graph &graph)
{
  for (int i = 0; i < ggfx->units_info.unit_count; ++i) 
  {
    sdf_unit &u = ggfx->units_arrays.units[i];
    add_sdf_unit(u, i);
  }

  dbg_add_octree_lines_(&root_, v3(0.0f), octree_max_level_);

  { // Update GPU-side buffers
    graph.add_buffer_update(RES("sdf-instances"), 
      sdf_render_instances_.data(), 0,  // Only update the parts whcih changed
      sizeof(sdf_render_instance) * sdf_render_instance_count_);

    graph.add_buffer_update(RES("sdf-units"), ggfx->units_arrays.units);
    graph.add_buffer_update(RES("sdf-list-nodes"), sdf_list_nodes_.data(), 0, sizeof(sdf_list_node) * sdf_list_nodes_.size());

    // We still want to be able to visualize the SDFs with the dumb SDF caster
    graph.add_buffer_update(RES("sdf-info-buffer"), &ggfx->units_info);
    graph.add_buffer_update(RES("sdf-add-buffer"), ggfx->units_arrays.add_units);
    graph.add_buffer_update(RES("sdf-sub-buffer"), ggfx->units_arrays.sub_units);
  }

  if (ggfx->vis_type == visualizer_type::raster_visualizer)
  { // Dispatch the render pass
    graph.add_render_pass(STG("sdf-rasterizer"))
      .add_color_attachment(RES("sdf-cast-target"), {0.0f, 0.0f, 0.0f, 0.0f})
      .add_depth_attachment(RES("sdf-cast-depth-map"), {1.0f})
      .prepare_commands([] (render_pass::prepare_package package)
      {
        auto tracker = package.graph->get_resource_tracker();

        tracker.prepare_buffer_for(RES("sdf-instances"),
          binding::type::storage_buffer, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);

        tracker.prepare_buffer_for(RES("sdf-list-nodes"),
          binding::type::storage_buffer, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);

        tracker.prepare_buffer_for(RES("sdf-units"),
          binding::type::uniform_buffer, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);

        tracker.prepare_buffer_for(RES("viewer-buffer"),
          binding::type::uniform_buffer, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
      }, nullptr)
      .draw_commands([] (render_pass::draw_package package)
      {
        auto tracker = package.graph->get_resource_tracker();

        // Get needed descriptor sets
        VkDescriptorSet instances_set = tracker.get_buffer(RES("sdf-instances"))
          .descriptor(binding::type::storage_buffer);
        VkDescriptorSet list_nodes_set = tracker.get_buffer(RES("sdf-list-nodes"))
          .descriptor(binding::type::storage_buffer);
        VkDescriptorSet units_set = tracker.get_buffer(RES("sdf-units"))
          .descriptor(binding::type::uniform_buffer);
        VkDescriptorSet viewer_set = tracker.get_buffer(RES("viewer-buffer"))
          .descriptor(binding::type::uniform_buffer);

        // Rasterize the cubes
        sdf_raster_pso_.bind(package.cmdbuf);
        sdf_raster_pso_.bind_descriptors(package.cmdbuf, instances_set, list_nodes_set, units_set, viewer_set);
        vkCmdDraw(package.cmdbuf, 36, sdf_render_instance_count_, 0, 0);
      }, nullptr);
  }
  else if (ggfx->vis_type == visualizer_type::caster_visualizer)
  {
    auto &pass = graph.add_compute_pass(STG("sdf-cast-pass"));
    pass.set_source("sdf_cast");
    pass.add_storage_image(RES("sdf-cast-target"));
    pass.add_uniform_buffer(RES("time-buffer"));
    pass.add_uniform_buffer(RES("sdf-info-buffer"));
    pass.add_uniform_buffer(RES("sdf-units"));
    pass.add_uniform_buffer(RES("sdf-add-buffer"));
    pass.add_uniform_buffer(RES("sdf-sub-buffer"));
    pass.add_uniform_buffer(RES("viewer-buffer"));
    pass.dispatch_waves(16, 16, 1, RES("sdf-cast-target"));
  }
  else
  {
    assert(false);
  }
}
