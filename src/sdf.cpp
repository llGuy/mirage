#include "sdf.hpp"
#include "ImGuizmo.h"
#include "debug_overlay.hpp"
#include "imgui.h"
#include "time.hpp"
#include "memory.hpp"
#include "core_render.hpp"
#include <glm/gtx/transform.hpp>

static constexpr u32 max_sdf_unit_data_size_() {
    return sizeof(u32) + 2 * max_sdf_unit_count * sizeof(sdf_unit);
}

static u32 add_manipulator_(const sdf_unit &u, u32 i, sdf_debug *debug) {
    u32 idx = debug->manipulators.size();
    debug->manipulators.push_back({ 
        glm::translate(v3(u.position)) * glm::scale(v3(u.scale)), i
    });
    return idx;
}

static void add_sdf_unit_(const sdf_unit &u, sdf_info *info, sdf_arrays *arrays, sdf_debug *debug) {
    u32 idx = info->unit_count++;
    arrays->units[idx] = u;
    arrays->units[idx].manipulator = add_manipulator_(u, idx, debug);

    switch (u.op) {
    case sdf_smooth_add: {
        u32 add_idx = info->add_unit_count++;
        arrays->add_units[add_idx] = idx;
    } break;

    case sdf_smooth_sub: {
        u32 sub_idx = info->sub_unit_count++;
        arrays->sub_units[sub_idx] = idx;
    } break;

    default: assert(false);
    }
}

static void sdf_manipulator_() {
    viewer_desc &viewer = ggfx->viewer;
    sdf_info *info = &ggfx->units_info;
    sdf_arrays *arrays = &ggfx->units_arrays;
    sdf_debug *debug = &ggfx->units_debug;

    if (ImGui::Button("Add SDF Unit")) {
        add_sdf_unit_({
            .position = v4(0.0, 0.0, 1.0, 1.0), .scale = v4(0.5f, 0.5f, 0.5f, 0.03),
            .type = debug->selected_shape, .op = debug->selected_op
        }, info, arrays, debug);

        debug->selected_manipulator = debug->manipulators.size()-1;
    }

    /* Add an SDF */
    const char *shape_names[] = { "Sphere", "Cube" };
    ImGui::Combo("Shape", (int *)&debug->selected_shape, shape_names, (int)sdf_type_none);

    const char *operation_names[] = { "Add", "Sub", "Intersect", "Smooth Add", "Smooth Sub", "Smooth Intersect" };
    ImGui::Combo("Operation", (int *)&debug->selected_op, operation_names, (int)op_type_none);

    ImGuizmo::OPERATION manip_ops[] = { ImGuizmo::TRANSLATE, ImGuizmo::ROTATE, ImGuizmo::SCALE };
    const char *manip_names[] = { "Translate", "Rotate", "Scale" };
    ImGui::Combo("Manipulation", &debug->manip_op_idx, manip_names, 3);
    debug->manip_op = manip_ops[debug->manip_op_idx];

    /* SDF List - select which to manipulate */
    char sdf_name[] = "- sdf0";
    ImGui::Text("SDF List:");

    for (int i = 0; i < debug->manipulators.size(); ++i) {
        sdf_manipulator *m = &debug->manipulators[i];
        sdf_unit *u = &arrays->units[m->idx];

        sdf_name[5] = '0' + i;
        if (ImGui::Selectable(sdf_name)) {
            debug->selected_manipulator = i;
        }

        // If we selected this unit, render manipulator
        if (i == debug->selected_manipulator) {
            m4x4 tx = glm::translate(v3(u->position)) * glm::scale(v3(u->scale));

            // Draw Cube too
            // ImGuizmo::DrawCubes(&viewer.view[0][0], &viewer.projection[0][0], &tx[0][0], 1);

            ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());

            ImGuizmo::Manipulate(&viewer.view[0][0], &viewer.projection[0][0], debug->manip_op, ImGuizmo::WORLD, &tx[0][0]);

            float translate[3], rotate[3], scale[3];
            ImGuizmo::DecomposeMatrixToComponents(&tx[0][0], translate, rotate, scale);

            u->position = v4(translate[0], translate[1], translate[2], 1.0f);
            u->scale = v4(scale[0], scale[1], scale[2], u->scale.w);
        }
    }
}

void init_sdf_units(render_graph &graph) {
    sdf_info *info = &ggfx->units_info;
    sdf_arrays *arrays = &ggfx->units_arrays;
    sdf_debug *debug = &ggfx->units_debug;

    arrays->units = mem_allocv<sdf_unit>(max_sdf_unit_count);
    arrays->add_units = mem_allocv<u32>(max_sdf_unit_count);
    arrays->sub_units = mem_allocv<u32>(max_sdf_unit_count);
    info->add_unit_count = 0;
    info->sub_unit_count = 0;
    info->unit_count = 0;

    ggfx->units_debug.selected_manipulator = -1;
    ggfx->units_debug.selected_op = sdf_smooth_add;

    // Hardcode the sdf_units
    add_sdf_unit_({
        .position = v4(-1.0, 0.0, 1.0, 1.0), .scale = v4(0.55),
        .type = sdf_sphere, .op = sdf_smooth_add
    }, info, arrays, debug);

#if 1
    add_sdf_unit_({
        .position = v4(-1.0, 0.0, 1.0, 1.0), .scale = v4(0.6, 0.2, 0.7, 0.03),
        .type = sdf_cube, .op = sdf_smooth_add
    }, info, arrays, debug);

    add_sdf_unit_({
        .position = v4(1.0, 0.0, 1.0, 1.0), .scale = v4(0.55f),
        .type = sdf_sphere, .op = sdf_smooth_add
    }, info, arrays, debug);

    add_sdf_unit_({
        .position = v4(1.0, 0.0, 1.0, 1.0), .scale = v4(0.6, 0.2, 0.7, 0.03),
        .type = sdf_cube, .op = sdf_smooth_add
    }, info, arrays, debug);
#endif

    graph.register_buffer(RES("sdf-units-buffer"))
        .configure({ .size = max_sdf_unit_count * sizeof(sdf_unit) });

    graph.register_buffer(RES("sdf-add-buffer"))
        .configure({ .size = max_sdf_unit_count * sizeof(u32) });

    graph.register_buffer(RES("sdf-sub-buffer"))
        .configure({ .size = max_sdf_unit_count * sizeof(u32) });

    graph.register_buffer(RES("sdf-info-buffer"))
        .configure({ .size = sizeof(sdf_info) });

    register_debug_overlay_client("SDF Units", sdf_manipulator_, true);

    init_sdf_octree();
}

void update_sdf_units(render_graph &graph) {
#if 1
    // Some objects are moving - offset by a sine wave
    float sn = glm::sin(gtime->current_time);
    float cn = glm::cos(gtime->current_time);

    sdf_unit *sphere1 = &ggfx->units_arrays.units[0];
    sdf_unit *sphere2 = &ggfx->units_arrays.units[2];

    sphere1->position.y = 0.5 + 0.3 * sn;
    sphere2->position.x = 1.0 + 0.3 * sn;
    sphere2->position.z = 1.0 + 0.3 * cn;
#endif

    clear_sdf_octree();
    update_sdf_octree();

    // Record command to update sdf units buffer
    graph.add_buffer_update(RES("sdf-info-buffer"), &ggfx->units_info);
    graph.add_buffer_update(RES("sdf-units-buffer"), ggfx->units_arrays.units);
    graph.add_buffer_update(RES("sdf-add-buffer"), ggfx->units_arrays.add_units);
    graph.add_buffer_update(RES("sdf-sub-buffer"), ggfx->units_arrays.sub_units);
}
