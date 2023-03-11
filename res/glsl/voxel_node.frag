#version 450

#include "sdf.glsl"
#include "viewer.glsl"

#define MAX_TRACE_ITERATIONS_PER_NODE_RAY 16

// This will allow us to see which SDF units to trace through
layout (location = 0) flat in uint in_sdf_list_node;
// NOT level - this stores 2^level
layout (location = 1) flat in float in_node_scale;
layout (location = 2) flat in vec3 in_node_pos;
layout (location = 3) in vec3 in_wstart;

layout (location = 0) out vec4 out_color;

/******************************** UNIFORMS ***********************************/
layout (set = 0, binding = 0) readonly buffer sdf_render_instances
{
  sdf_render_instance data[];
} usdf_instances;

layout (set = 1, binding = 0) readonly buffer sdf_list_nodes
{
  sdf_list_node data[];
} usdf_list_nodes;

// Holds information about each SDF unit
layout (set = 2, binding = 0) uniform sdf_units
{
  sdf_unit data[max_sdf_unit_count];
} usdf_units;

// Camera
layout (set = 3, binding = 0) uniform viewer_data 
{
  viewer_desc data;
} uviewer;



/******************************** RAY TRACE ***********************************/
struct node_ray_info
{
  vec3 wstart;
  vec3 wend;
  float len;

  // Normalized direction of the ray
  vec3 wdir;
};

// Get further intersection of box in certain ray direction
float box_max_intersect(in vec3 ray_origin, in vec3 ray_dir, in vec3 minpos, in vec3 maxpos)
{
  vec3 inverse_dir = 1.0 / ray_dir;
  vec3 tbot = inverse_dir * (minpos - ray_origin);
  vec3 ttop = inverse_dir * (maxpos - ray_origin);
  vec3 tmin = min(ttop, tbot);
  vec3 tmax = max(ttop, tbot);

  vec2 traverse = min(tmax.xx, tmax.yz);
  float traversehi = min(traverse.x, traverse.y);

  return traversehi;
}

node_ray_info get_ray_info()
{
  // WSTART is the ray starting position whereas IN_NODE_POS is the cube's low min position
  vec3 wdir = normalize(in_wstart - uviewer.data.wposition);
  vec3 node_max_pos = in_node_pos + vec3(in_node_scale);
  float t = box_max_intersect(in_wstart, wdir, in_node_pos, node_max_pos);

  node_ray_info ret;
  ret.wstart = in_wstart;
  ret.wend = in_wstart + wdir * t;
  ret.wdir = wdir;
  ret.len = t;

  return ret;
}

/******************************** SDF SAMPLING ********************************/
void map_sdf(inout float d, uint sdf_unit_idx, in vec3 wray_pos)
{
  sdf_unit u = usdf_units.data[sdf_unit_idx];
  float d_unit = 1e10;

  if (u.type == sdf_sphere)
    d_unit = sphere(wray_pos - u.position.xyz, 0.55);
  else if (u.type == sdf_cube)
    d_unit = cube(wray_pos - u.position.xyz, u.scale.xyz, u.scale.w);

  d = op_smooth_union(d_unit, d, 0.25);
}

// Returns the signed distance at a given position
float sdf_sample_pos(in vec3 pos)
{
  float d = 1e10;

  // Loop through SDFs that affect this node
  uint current_node_idx = in_sdf_list_node;
  sdf_list_node current_node = usdf_list_nodes.data[in_sdf_list_node];
  // Will use this as the number of elements left
  int unit_count = int(current_node.items[0]); // 0th element of the first list node is the count
  uint current_local_idx = 1;

  while (unit_count > 0 || current_local_idx < 15)
  {
    map_sdf(d, current_node.items[current_local_idx], pos);

    --unit_count;
    ++current_local_idx;
  }

  current_local_idx = 0;
  current_node = usdf_list_nodes.data[current_node.items[15]];

  while (unit_count > 0)
  {
    if (current_local_idx == 15)
    {
      current_local_idx = 0;
      current_node = usdf_list_nodes.data[current_node.items[15]];
    }

    map_sdf(d, current_node.items[current_local_idx], pos);

    --unit_count;
    ++current_local_idx;
  }

  return d;
}

struct sdf_sample_info
{
  // For now, just experiment with opaque objects, afterwards, let's play
  // with transparent / cloudy objects
  bool is_opaque;
  vec3 wpos;

};

sdf_sample_info sdf_sample_ray(in node_ray_info info)
{
  sdf_sample_info smp;
  smp.is_opaque = false;

  float t = 0.0;
  for (int i = 0; i < MAX_TRACE_ITERATIONS_PER_NODE_RAY && t < info.len; ++i)
  {
    vec3 current_pos = info.wstart + t * info.wdir;
    float d = sdf_sample_pos(current_pos);
    
    if (abs(d) < 0.001)
    {
      // We got a hit
      smp.is_opaque = true;
      smp.wpos = current_pos;
      break;
    }

    // Advance by the necessary amount
    t += d;
  }

  return smp;
}



void main() 
{
  node_ray_info ray = get_ray_info();
  sdf_sample_info smp = sdf_sample_ray(ray);

  if (!smp.is_opaque)
    discard;

  // For now just set some random color
  out_color = vec4(1.0);

  // Update the depth buffer
  vec4 ndc = uviewer.data.view_projection * vec4(smp.wpos, 1.0);
  ndc.xyz /= ndc.w;

  gl_FragDepth = ndc.z;
}
