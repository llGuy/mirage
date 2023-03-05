#include <imgui.h>
#include <ImGuizmo.h>
#include "core_render.hpp"
#include "debug_overlay.hpp"
#include "heap_array.hpp"
#include "memory.hpp"
#include "bits.hpp"
#include "render_context.hpp"
#include "sdf.hpp"
#include "viewer.hpp"

#define OCTREE_NODE_WIDTH 2
#define MAX_SDF_RENDER_INSTANCES 1024
// (roughly) Pixel width/height or each octree node
#define OCTREE_NODE_SCREEN_SIZE 128

v2 pos_to_pixel_coords(v2 ndc) {
    v2 extent = v2((float)gctx->swapchain_extent.width, (float)gctx->swapchain_extent.height);
    v2 uv = (ndc + v2(1.0f)) / 2.0f;

    return uv * extent;
}

v2 dir_to_pixel_coords(v2 ndc) {
    v2 extent = v2((float)gctx->swapchain_extent.width, (float)gctx->swapchain_extent.height);
    v2 uv = ndc / 2.0f;

    return uv * extent;
}

union octree_info_extracter {
    // To use this, set bits to the u32 and use the members below
    u32 bits;

    struct {
        // Does this contain any information whatsoever
        u32 is_empty : 1;

        // Is this a leaf (i.e., does this contain SDF information)
        // Otherwise, this is a node which points to another octree node
        u32 is_leaf : 1;
        
        // If is_leaf == 1, then this is a location into the SDF information pool
        // If is_leaf == 0, then this is a location into the octree node pool
        u32 loc : 30;
    };
};

struct octree_node {
    // Nodes of the octree
    u32 nodes[OCTREE_NODE_WIDTH] /* Z */
             [OCTREE_NODE_WIDTH] /* Y */
             [OCTREE_NODE_WIDTH] /* X */;
};

struct sdf_list_node {
    union {
        struct {
            u32 count;
            u32 sdf_ids[14];
            u32 next;
        } first_elem;

        struct {
            u32 sdf_ids[15];
            u32 next;
        } inner_elem;
    };
};

/* For now, just store this stuff - in future, will compact */
struct sdf_render_instance {
    u32 sdf_list_node_idx;
    v3 wposition;
    u32 level; // Enforces the scale of the cube
};

template <typename T>
class arena_pool {
public:
    arena_pool() = default;

    arena_pool(u32 size) 
    : pool_size_(size) {
        pool_size_ = size;
        pool_ = mem_allocv<octree_node>(size);
        first_free_ = nullptr;
        reached_ = 0;
    }

    T *alloc() {
        if (first_free_) {
            T *new_free_node = first_free_;
            first_free_ = next(first_free_);
            return new_free_node;
        }
        else {
            assert(reached_ < pool_size_);
            return &pool_[reached_++];
        }
    }

    void clear() {
        reached_ = 0;
        first_free_ = nullptr;
    }

    u32 get_node_idx(T *node) {
        return node - pool_;
    }

    T *get_node(u32 idx) {
        return &pool_[idx];
    }

    void free(T *node) {
        next(node) = first_free_;
        first_free_ = node;
    }

    T *&next(T *node) {
        return *(T **)node;
    }

private:
    T *pool_;
    T *first_free_;

    // Which node have we reached
    u32 reached_;
    // How many nodes are there
    u32 pool_size_;
};

// Pool of octree nodes
static arena_pool<octree_node> octree_nodes_;

// Pool of SDF leaf nodes

static arena_pool<sdf_list_node> sdf_list_nodes_;
// List of SDF cube nodes
static heap_array<sdf_render_instance> sdf_render_instances_;

// Root node
static octree_node *root_;

struct sdf_octree_debug {
    iv2 count;
    iv2 px_start;
    iv2 px_rng;
} debug_;

static void sdf_octree_gen_debug_proc_() {
    ImGui::Text("Count: %d %d\n", debug_.count.x, debug_.count.y);
    ImGui::Text("Pixel Start: %d %d\n", debug_.px_start.x, debug_.px_start.y);
    ImGui::Text("Pixel Range: %d %d\n", debug_.px_rng.x, debug_.px_rng.y);
}

void init_sdf_octree() {
    octree_nodes_ = arena_pool<octree_node>(1024);
    root_ = octree_nodes_.alloc();

    sdf_render_instances_ = heap_array<sdf_render_instance>(MAX_SDF_RENDER_INSTANCES);

    register_debug_overlay_client("SDF Octree Gen", sdf_octree_gen_debug_proc_, true);
}

void clear_sdf_octree() {
    octree_nodes_.clear();
    sdf_list_nodes_.clear();
}

void add_sdf_unit(const sdf_unit &u) {
    viewer_desc &viewer = ggfx->viewer;
    
    v3 scale = v3(u.scale) + v3(0.1f);
    v3 low = v3(u.position) - scale;

    v3 bound_corners[] = {
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

    for (int i = 0; i < 8; ++i) {
        v4 clip = viewer.view_projection * v4(bound_corners[i], 1.0f);
        v2 screen_space_coord = v2(clip) / clip.w;

        ss_low.x = glm::min(screen_space_coord.x, ss_low.x);
        ss_low.y = glm::min(screen_space_coord.y, ss_low.y);

        ss_high.x = glm::max(screen_space_coord.x, ss_high.x);
        ss_high.y = glm::max(screen_space_coord.y, ss_high.y);
    }

    v2 rng = v2(ss_high.x - ss_low.x, ss_high.y - ss_low.y);

    if (ggfx->units_debug.selected_manipulator == u.manipulator) {
        add_debug_rectangle({
            .color = v4(0.0f, 0.0f, 1.0f, 0.4f),
            .positions = { 
                v2(ss_low.x, ss_low.y),
                v2(ss_low.x, ss_low.y + rng.y),
                v2(ss_low.x + rng.x, ss_low.y + rng.y),
                v2(ss_low.x + rng.x, ss_low.y)
            }
        });

        v2 px_rng = dir_to_pixel_coords(rng);
        v2 px_low = pos_to_pixel_coords(ss_low);

        iv2 square_count = iv2(glm::ceil(px_rng / v2(OCTREE_NODE_SCREEN_SIZE)));

        debug_.px_rng = px_rng;
        debug_.px_start = px_low;
        debug_.count = square_count;
    }
}

void update_sdf_octree() {
    for (int i = 0; i < ggfx->units_info.unit_count; ++i) {
        sdf_unit &u = ggfx->units_arrays.units[i];
        add_sdf_unit(u);
    }
}
