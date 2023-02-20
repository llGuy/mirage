#include "sdf.hpp"
#include "time.hpp"
#include "memory.hpp"
#include "core_render.hpp"

/* Divide the space into voxels (maybe in frustum space),
 * Each voxel/froxel stores an array of the sdf_units which
 * affect that space. */

static constexpr u32 max_sdf_unit_data_size_() {
    return sizeof(u32) + 2 * max_sdf_unit_count * sizeof(sdf_unit);
}

static void add_sdf_unit_(const sdf_unit &u) {
    switch (u.op) {
    case sdf_smooth_add: {
        ggfx->sdf_units->add_data[ggfx->sdf_units->add_count++] = u;
    } break;

    case sdf_smooth_sub: {
        ggfx->sdf_units->sub_data[ggfx->sdf_units->sub_count++] = u;
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

void init_sdf_units(render_graph &graph) {
    ggfx->sdf_units = mem_alloc<sdf_unit_array>();

    // Hardcode the sdf_units
    add_sdf_unit_({
        v4(-1.0, 0.0, 1.0, 1.0), v4(0.6, 0.2, 0.7, 0.55),
        sdf_sphere, sdf_smooth_add
    });

    add_sdf_unit_({
        v4(-1.0, 0.0, 1.0, 1.0), v4(0.6, 0.2, 0.7, 0.1),
        sdf_cube, sdf_smooth_add
    });

    add_sdf_unit_({
        v4(1.0, 0.0, 1.0, 1.0), v4(0.6, 0.2, 0.7, 0.55),
        sdf_sphere, sdf_smooth_add
    });

    add_sdf_unit_({
        v4(1.0, 0.0, 1.0, 1.0), v4(0.6, 0.2, 0.7, 0.1),
        sdf_cube, sdf_smooth_add
    });

    graph.register_buffer(RES("sdf-units-buffer"))
        .configure({ .size = max_sdf_unit_data_size_() });
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
