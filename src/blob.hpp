#pragma once

#include "types.hpp"
#include "render_graph.hpp"

constexpr u32 max_blob_count = 32;

enum blob_type {
    sdf_sphere, sdf_cube
};

enum op_type {
    sdf_add, 
    sdf_sub,
    sdf_intersect,
    sdf_smooth_add,
    sdf_smooth_sub,
    sdf_smooth_intersect
};

struct blob {
    // Add rotations later
    v4 position;
    v4 scale;
    u32 type;
    u32 op;
    u32 pad[2];
};

struct blob_array {
    u32 sub_count;
    u32 add_count;
    u32 pad[2];

    blob sub_data[max_blob_count];
    blob add_data[max_blob_count];
};

void init_blobs();
void update_blobs(render_graph &graph);
