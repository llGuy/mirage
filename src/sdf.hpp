#pragma once

#include "types.hpp"
#include "render_graph.hpp"
#include "debug_overlay.hpp"

constexpr u32 max_sdf_unit_count = 32;

enum sdf_type {
    sdf_sphere, sdf_cube, sdf_type_none
};

enum op_type {
    sdf_add, 
    sdf_sub,
    sdf_intersect,
    sdf_smooth_add,
    sdf_smooth_sub,
    sdf_smooth_intersect,
    op_type_none
};

struct sdf_unit {
    // Add rotations later
    v4 position;
    v4 scale;
    u32 type;
    u32 op;

    u32 manipulator;
    u32 pad;
};

struct sdf_manipulator {
    m4x4 tx;
    u32 shape_op;
    u32 idx;
};

struct sdf_unit_array {
    u32 sub_count;
    u32 add_count;
    u32 pad[2];

    sdf_unit sub_data[max_sdf_unit_count];
    sdf_unit add_data[max_sdf_unit_count];

    std::vector<sdf_manipulator> manipulators;
    s32 selected_manipulator;
    op_type selected_op;
    sdf_type selected_shape;
    ImGuizmo::OPERATION manip_op;
    int manip_op_idx;
};

void init_sdf_units(render_graph &graph);
void update_sdf_units(render_graph &graph);
