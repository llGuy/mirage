#ifndef SDF_GLSL
#define SDF_GLSL

#define max_sdf_unit_count 32

#define sdf_sphere 0x0
#define sdf_cube   0x1

#define sdf_union               0x0
#define sdf_sub                 0x1
#define sdf_intersect           0x2
#define sdf_smooth_union        0x3
#define sdf_smooth_sub          0x4
#define sdf_smooth_intersect    0x5

struct sdf_unit 
{
  // Add rotations later
  vec4 position;
  vec4 scale;
  uint type;
  uint op;
  uint pad0;
  uint pad1;
};

struct sdf_render_instance
{
  vec4 wposition;

  uint sdf_list_node_idx;
  uint level;

  uint dump1, dump2;
};

const uint invalid_sdf_id = 0xFFFFFFFF;

struct sdf_list_node
{
  /* If this is the first node in the list, then the format is as follows:
   * - count = items[0], elements[] = items[1...14], next = items[15] 
   *
   * If this is anything but the first node in the list, then the format is:
   * - elements[] = items[0, 14], next = items[15] */
  uint items[16];
};

/* Some basic SDF math operations */
float op_union(float d1, float d2) 
{
  return min(d1, d2);
}

float op_sub(float d1, float d2) 
{ 
  return max(-d1, d2);
}

float op_intersect(float d1, float d2) 
{
  return max(d1, d2);
}

float op_smooth_union(float d1, float d2, float k) 
{
  float h = max(k - abs(d1 - d2), 0.0);
  return min(d1, d2) - h * h * 0.25 / k;
}

float op_smooth_sub(float d1, float d2, float k) 
{
  return -op_smooth_union(d1, -d2, k);
}

float op_smooth_intersect(float d1, float d2, float k) 
{
  return -op_smooth_union(-d1, -d2, k);
}

float sphere(in vec3 p, in float r) 
{
  return length(p) - r;
}

float cube(vec3 p, vec3 b, float r) 
{
  vec3 d = abs(p) - b;
  return min(max(d.x, max(d.y, d.z)), 0.0) + length(max(d, 0.0)) - r;
}

#endif
