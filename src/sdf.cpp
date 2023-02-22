#include "sdf.hpp"
#include "ImGuizmo.h"
#include "debug_overlay.hpp"
#include "imgui.h"
#include "time.hpp"
#include "memory.hpp"
#include "core_render.hpp"
#include <glm/gtx/transform.hpp>

/* Divide the space into voxels (maybe in frustum space),
 * Each voxel/froxel stores an array of the sdf_units which
 * affect that space. */

static constexpr u32 max_sdf_unit_data_size_() {
    return sizeof(u32) + 2 * max_sdf_unit_count * sizeof(sdf_unit);
}

static u32 add_manipulator_(const sdf_unit &u, u32 i) {
    u32 idx = ggfx->sdf_units->manipulators.size();
    ggfx->sdf_units->manipulators.push_back({ 
        ImGuizmo::TRANSLATE,
        glm::translate(v3(u.position)) * glm::scale(v3(u.scale)),
        u.op, i
    });
    return idx;
}

static void add_sdf_unit_(const sdf_unit &u) {
    switch (u.op) {
    case sdf_smooth_add: {
        u32 idx = ggfx->sdf_units->add_count++;
        ggfx->sdf_units->add_data[idx] = u;
        ggfx->sdf_units->add_data[idx].manipulator = add_manipulator_(u, idx);
    } break;

    case sdf_smooth_sub: {
        u32 idx = ggfx->sdf_units->add_count++;
        ggfx->sdf_units->sub_data[idx] = u;
        ggfx->sdf_units->sub_data[idx].manipulator = add_manipulator_(u, idx);
    } break;

    default: assert(false);
    }
}

static sdf_unit *get_sdf_unit_(op_type type, u32 idx) {
    switch (type) {
    case sdf_smooth_add: {
        return &ggfx->sdf_units->add_data[idx];
    } break;

    case sdf_smooth_sub: {
        return &ggfx->sdf_units->sub_data[idx];
    } break;

    default: assert(false);
    }
}

static void sdf_manipulator_() {
    ImGuiIO &io = ImGui::GetIO();

    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

    viewer_desc &viewer = ggfx->viewer;

    /* Add an SDF */

    /* SDF List - select which to manipulate */
    char sdf_name[] = "- sdf0";
    ImGui::Text("SDF List:");

    for (int i = 0; i < ggfx->sdf_units->manipulators.size(); ++i) {
        sdf_manipulator *m = &ggfx->sdf_units->manipulators[i];
        sdf_unit *u;

        if (m->shape_op == sdf_smooth_add)
            u = &ggfx->sdf_units->add_data[m->idx];
        else
            u = &ggfx->sdf_units->sub_data[m->idx];

        sdf_name[5] = '0' + i;
        if (ImGui::Selectable(sdf_name)) {
            ggfx->sdf_units->selected_manipulator = i;
        }

        // If we selected this unit, render manipulator
        if (i == ggfx->sdf_units->selected_manipulator) {
            m4x4 tx = glm::translate(v3(u->position)) * glm::scale(v3(u->scale));

            ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());

            ImGuizmo::Manipulate(&viewer.view[0][0], &viewer.projection[0][0], m->op, ImGuizmo::WORLD, &tx[0][0]);

            float translate[3], rotate[3], scale[3];
            ImGuizmo::DecomposeMatrixToComponents(&tx[0][0], translate, rotate, scale);

            u->position = v4(translate[0], translate[1], translate[2], 1.0f);
        }
    }
}

void init_sdf_units(render_graph &graph) {
    ggfx->sdf_units = mem_alloc<sdf_unit_array>();

    ggfx->sdf_units->selected_manipulator = -1;

    // Hardcode the sdf_units
    add_sdf_unit_({
        .position = v4(-1.0, 0.0, 1.0, 1.0), .scale = v4(0.6, 0.2, 0.7, 0.55),
        .type = sdf_sphere, .op = sdf_smooth_add
    });

    add_sdf_unit_({
        .position = v4(-1.0, 0.0, 1.0, 1.0), .scale = v4(0.6, 0.2, 0.7, 0.1),
        .type = sdf_cube, .op = sdf_smooth_add
    });

    add_sdf_unit_({
        .position = v4(1.0, 0.0, 1.0, 1.0), .scale = v4(0.6, 0.2, 0.7, 0.55),
        .type = sdf_sphere, .op = sdf_smooth_add
    });

    add_sdf_unit_({
        .position = v4(1.0, 0.0, 1.0, 1.0), .scale = v4(0.6, 0.2, 0.7, 0.1),
        .type = sdf_cube, .op = sdf_smooth_add
    });

    graph.register_buffer(RES("sdf-units-buffer"))
        .configure({ .size = max_sdf_unit_data_size_() });

    register_debug_overlay_client("SDF Units", sdf_manipulator_);
}

void update_sdf_units(render_graph &graph) {
    // Some objects are moving - offset by a sine wave
    float sn = glm::sin(gtime->current_time);
    float cn = glm::cos(gtime->current_time);

    sdf_unit *sphere1 = get_sdf_unit_(sdf_smooth_add, 0);
    sdf_unit *sphere2 = get_sdf_unit_(sdf_smooth_add, 2);

    sphere1->position.y = 0.5 + 0.3 * sn;
    sphere2->position.x = 1.0 + 0.3 * sn;
    sphere2->position.z = 1.0 + 0.3 * cn;

    // Record command to update sdf units buffer
    graph.add_buffer_update(RES("sdf-units-buffer"), ggfx->sdf_units);
}
