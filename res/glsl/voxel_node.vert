/* The way the SDFs get rendered (just visibility) is as follows:
 * - We put all the SDF render instances in a vertex buffer where
 *   each instance corresponds to an actual "instance" in vkCmdDraw
 * - They get read into this shader which uses that SDF render instance
 *   information to rasterize a cube
 * - The fragment shader runs the tracing code just through the volume
 *   of the cube */

#version 450

#extension GL_EXT_scalar_block_layout : require

#include "sdf.glsl"
#include "viewer.glsl"

// Outputs just the SDF list node
layout (location = 0) flat out uint out_sdf_list_node;

// Get vertex information from here
layout (set = 0, binding = 0) readonly buffer sdf_render_instances
{
  sdf_render_instance data[];
} usdf_instances;

// Holds information about each SDF unit
layout (set = 1, binding = 0) uniform sdf_units
{
  sdf_unit data[];
} usdf_units;

// Camera
layout (set = 2, binding = 0) uniform viewer_data 
{
  viewer_desc data;
} uviewer;

const vec3 positions[] = vec3[]
(
  // front
  vec3(0.0, 0.0, 1.0),
  vec3(1.0, 0.0, 1.0),
  vec3(1.0, 1.0, 1.0),
  vec3(0.0, 1.0, 1.0),
  // back
  vec3(0.0, 0.0, 0.0),
  vec3(1.0, 0.0, 0.0),
  vec3(1.0, 1.0, 0.0),
  vec3(0.0, 1.0, 0.0)
);

const uint indices[] = uint[]
(
  // front
  0, 1, 2,
  2, 3, 0,
  // right
  1, 5, 6,
  6, 2, 1,
  // back
  7, 6, 5,
  5, 4, 7,
  // left
  4, 0, 3,
  3, 7, 4,
  // bottom
  4, 5, 1,
  1, 0, 4,
  // top
  3, 2, 6,
  6, 7, 3
);

void main() 
{
  // Get local space cube vertices
  uint idx = indices[gl_VertexIndex];
  vec3 vtx = positions[idx];

  // Transform the vertices to world space
  sdf_render_instance inst = usdf_instances.data[gl_InstanceIndex];
  float scale = exp2(float(inst.level));
  vtx = inst.wposition.xyz + scale * vtx;

  // Finalize
  gl_Position = uviewer.data.projection * uviewer.data.view * vec4(vtx, 1.0);
  gl_Position.y *= -1.0f;

  out_sdf_list_node = inst.sdf_list_node_idx;
}
