#include "blob.hpp"
#include "time.hpp"
#include "buffer.hpp"
#include "memory.hpp"
#include "core_render.hpp"

/* Divide the space into voxels (maybe in frustum space),
 * Each voxel/froxel stores an array of the blobs which
 * affect that space. */

static constexpr u32 max_blob_data_size_() {
    return sizeof(u32) + 2 * max_blob_count * sizeof(blob);
}

static void add_blob_(const blob &b) {
    switch (b.op) {
    case sdf_smooth_add: {
        ggfx->blobs->add_data[ggfx->blobs->add_count++] = b;
    } break;

    case sdf_smooth_sub: {
        ggfx->blobs->sub_data[ggfx->blobs->sub_count++] = b;
    } break;

    default: assert(false);
    }
}

static blob *get_blob_(op_type type, u32 idx) {
    switch (type) {
    case sdf_smooth_add: {
        return &ggfx->blobs->add_data[idx];
    } break;

    case sdf_smooth_sub: {
        return &ggfx->blobs->sub_data[idx];
    } break;

    default: assert(false);
    }
}

void init_blobs() {
    ggfx->blob_data = make_uniform_buffer(max_blob_data_size_());
    ggfx->blobs = mem_alloc<blob_array>();

    // Hardcode the blobs
    add_blob_({
        v4(-1.0, 0.0, 1.0, 1.0), v4(0.6, 0.2, 0.7, 0.55),
        sdf_sphere, sdf_smooth_add
    });

    add_blob_({
        v4(-1.0, 0.0, 1.0, 1.0), v4(0.6, 0.2, 0.7, 0.1),
        sdf_cube, sdf_smooth_add
    });

    add_blob_({
        v4(1.0, 0.0, 1.0, 1.0), v4(0.6, 0.2, 0.7, 0.55),
        sdf_sphere, sdf_smooth_add
    });

    add_blob_({
        v4(1.0, 0.0, 1.0, 1.0), v4(0.6, 0.2, 0.7, 0.1),
        sdf_cube, sdf_smooth_add
    });
}

void update_blobs(render_graph &graph) {
    // Some objects are moving - offset by a sine wave
    float sn = glm::sin(gtime->current_time);
    float cn = glm::cos(gtime->current_time);

    blob *sphere1 = get_blob_(sdf_smooth_add, 0);
    blob *sphere2 = get_blob_(sdf_smooth_add, 2);
    // blob *sphere3 = get_blob_(sdf_smooth_add, 2);

    sphere1->position.y = 0.5 + 0.3 * sn;
    // sphere2->position.x = 0.6 + 0.3 * an;
    sphere2->position.x = 1.0 + 0.3 * sn;
    sphere2->position.z = 1.0 + 0.3 * cn;

    ggfx->blob_data.update(graph, 0, max_blob_data_size_(), ggfx->blobs);
}
