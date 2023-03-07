#include <imgui.h>
#include <ImGuizmo.h>
#include "core_render.hpp"
#include "debug_overlay.hpp"
#include "glm/common.hpp"
#include "glm/exponential.hpp"
#include "heap_array.hpp"
#include "math_util.hpp"
#include "memory.hpp"
#include "bits.hpp"
#include "render_context.hpp"
#include "sdf.hpp"
#include "viewer.hpp"

#define OCTREE_NODE_WIDTH 2
#define MAX_SDF_RENDER_INSTANCES 1024
// (roughly) Pixel width/height or each octree node
#define OCTREE_NODE_SCREEN_SIZE 128
#define OCTREE_NODE_SCALE (1.0f)

// Supported voxel node sizes: 1, 2, 4, 8, 16, 32, 64, etc...

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

typedef u32 node_info;

bool is_node_empty(node_info n) {
    octree_info_extracter *e = (octree_info_extracter *)&n;
    return e->is_empty;
}

bool is_node_leaf(node_info n) {
    octree_info_extracter *e = (octree_info_extracter *)&n;
    return e->is_leaf;
}

u32 node_loc(node_info n) {
    octree_info_extracter *e = (octree_info_extracter *)&n;
    return e->loc;
}

void set_is_node_empty(node_info n, bool value) {
    octree_info_extracter *e = (octree_info_extracter *)&n;
    e->is_empty = value;
}

void set_is_node_leaf(node_info n, bool value) {
    octree_info_extracter *e = (octree_info_extracter *)&n;
    e->is_leaf = value;
}

void set_node_loc(node_info n, u32 loc) {
    octree_info_extracter *e = (octree_info_extracter *)&n;
    e->loc = loc;
}

node_info create_node_info(bool is_empty, bool is_leaf, u32 loc) {
    octree_info_extracter e;
    e.is_empty = is_empty;
    e.is_leaf = is_leaf;
    e.loc = loc;

    return e.bits;
}

struct octree_node {
    // Nodes of the octree
    node_info child_nodes[OCTREE_NODE_WIDTH] /* Z */
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
        memset(pool_, 0, sizeof(T) * size);
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

// Max level of the octree
static float octree_max_level_;

// Root node
static node_info root_;

struct sdf_octree_debug {
    v3 cube_center;
    v3 cube_scale;

    float node_cube_width;
    float lod_level;
    float screen_space_level;
    float final_level;

    v3 cube_start;
    v3 cube_end;
    v3 count;
} debug_;

static void sdf_octree_gen_debug_proc_() {
    viewer_desc &viewer = ggfx->viewer;

    float cube_width = OCTREE_NODE_SCALE * glm::pow(2.0f, debug_.final_level);
    ImGui::Text("World Cube Width: %f", cube_width);
    ImGui::Text("World Cube Level: %f", debug_.final_level);
    ImGui::Text("Screen Space Level: %f", debug_.screen_space_level);
    ImGui::Text("LOD Level: %f", debug_.lod_level);
    ImGui::Text("Count: %f %f %f", debug_.count.x, debug_.count.y, debug_.count.z);
    ImGui::Text("Cube Start: %f %f %f", debug_.cube_start.x, debug_.cube_start.y, debug_.cube_start.z);
    ImGui::Text("Cube End: %f %f %f", debug_.cube_end.x, debug_.cube_end.y, debug_.cube_end.z);

    m4x4 tx = glm::translate(debug_.cube_center) * glm::scale(v3(cube_width/2.0f));
    ImGuizmo::DrawCubes(&viewer.view[0][0], &viewer.projection[0][0], &tx[0][0], 1);
}

void init_sdf_octree() {
    octree_max_level_ = 32.0f;

    octree_nodes_ = arena_pool<octree_node>(1024);
    root_ = create_node_info(false, false, octree_nodes_.get_node_idx(octree_nodes_.alloc()));

    sdf_render_instances_ = heap_array<sdf_render_instance>(MAX_SDF_RENDER_INSTANCES);

    register_debug_overlay_client("SDF Octree Gen", sdf_octree_gen_debug_proc_, true);
}

void clear_sdf_octree() {
    octree_nodes_.clear();
    sdf_list_nodes_.clear();
}

// Get level from LOD
static float find_sdf_unit_level_lod_(const sdf_unit &u) {
    viewer_desc &viewer = ggfx->viewer;

    float average_view_z = (viewer.view * v4(v3(u.position), 1.0f)).z;

    // What the fuckk do I do here
    // Find out world space length of the boxes that will enclose the shape
    m4x4 &p_tx = viewer.projection;

    // Width of the node in screen space
    float ss_node_width = ((float)OCTREE_NODE_SCREEN_SIZE / (float)gctx->swapchain_extent.width) * 2.0f;
    float ws_node_width = ss_node_width * (p_tx[3][2] * average_view_z) / p_tx[0][0];
    float n = glm::max(0.0f, glm::ceil(glm::log2(ws_node_width / OCTREE_NODE_SCALE)));

    return n;
}

// Get level from screen space size
static float find_sdf_unit_level_screen_size_(const sdf_unit &u) {
    // First find the screen space width of the encapsulating cube
    viewer_desc &viewer = ggfx->viewer;
    float average_view_z = (viewer.view * v4(v3(u.position), 1.0f)).z;

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

        ss_low = glm::min(screen_space_coord, ss_low);
        ss_high = glm::max(screen_space_coord, ss_high);
    }

    v2 rng = v2(ss_high.x - ss_low.x, ss_high.y - ss_low.y);
    v2 px_rng = dir_to_pixel_coords(rng);

    float level = 0.0f;

    // Figure out which range is smaller, this will dictacte the screen space width of cube
    if (px_rng.x < px_rng.y) {
        // Use X component for cube width
        m4x4 &p_tx = viewer.projection;
        float ss_node_width = rng.x;
        float ws_node_width = ss_node_width * (p_tx[3][2] * average_view_z) / p_tx[0][0];
        level = glm::max(0.0f, glm::ceil(glm::log2(ws_node_width / OCTREE_NODE_SCALE)));
    }
    else {
        // Use Y componentf or cube width
        m4x4 &p_tx = viewer.projection;
        float ss_node_width = rng.y;
        float ws_node_width = ss_node_width * (p_tx[3][2] * average_view_z) / p_tx[1][1];
        level = glm::max(0.0f, glm::ceil(glm::log2(ws_node_width / OCTREE_NODE_SCALE)));
    }

    return level;
}

float find_sdf_unit_level(const sdf_unit &u) {
    float level_lod = find_sdf_unit_level_lod_(u);
    float level_screen_space = find_sdf_unit_level_screen_size_(u);
    return glm::min(level_screen_space, level_lod);
}

// More optimized for inserting multiple nodes which are close to eachother
void insert_batched_sdf_nodes_into_octree() {

}

// The position is in octree space (where 0,0,0 is at the octree origin)
void insert_sdf_node_into_octree(float level, v3 pos) {
    int dst_level = (int)level;
    float scale = glm::pow(2.0f, level);

    node_info *current_node = &root_;
    v3 center = v3(0.0f);
    bool found = false;

    for (int i = (int)octree_max_level_; i >= 0; --i) {
        if (dst_level == i || is_node_leaf(*current_node)) {
            found = true;
            break;
        }

        // If the node is empty (i.e. doesn't point to anything), then, create a new one
        if (is_node_empty(*current_node)) {
            // Create a new node
            node_info new_node = create_node_info(
                false, false, octree_nodes_.get_node_idx(octree_nodes_.alloc()));
            *current_node = new_node;
        }

        // Current subdivision level
        float current_level = (float)i;
        float node_size = glm::pow(2.0f, current_level);
        float child_node_size = node_size / 2.0f;

        // Figure out which octant the position is in
        v3 level_start_pos = center - v3(child_node_size);
        v3 offset = pos - level_start_pos;

        assert(offset.x >= 0.0f && offset.y >= 0.0f && offset.z >= 0.0f &&
               offset.x < node_size && offset.y < node_size && offset.z < node_size);

        offset /= child_node_size;

        // This is the index into the octree node 3D array
        iv3 child_idx = iv3(glm::floor(offset));

        assert(child_idx.x >= 0 && child_idx.y >= 0 && child_idx.z >= 0 &&
               child_idx.x < OCTREE_NODE_WIDTH && child_idx.y < OCTREE_NODE_WIDTH && child_idx.z < OCTREE_NODE_WIDTH);

        center = level_start_pos + v3(child_idx) * child_node_size + v3(child_node_size/2.0f);

        octree_node *node_ptr = octree_nodes_.get_node(node_loc(*current_node));
        current_node = &node_ptr->child_nodes[child_idx.x][child_idx.y][child_idx.z];
    }

    assert(found);

    // Now do some voodoo shit
}

// Add an SDF unit to the octree
void add_sdf_unit(const sdf_unit &u) {
    v3 scale = v3(u.scale) + v3(0.1f);
    v3 low = v3(u.position) - scale;
    v3 high = v3(u.position) + scale;

    float level_lod = find_sdf_unit_level_lod_(u);
    float level_screen_space = find_sdf_unit_level_screen_size_(u);
    float level = glm::min(level_screen_space, level_lod);

    // Width of the cubes which will encompass this SDF unit
    float cube_width = OCTREE_NODE_SCALE * glm::pow(2.0f, level);

    v3 cube_start = floor_to(low, cube_width);
    v3 cube_end = ceil_to(high, cube_width);
    v3 rng = cube_end - cube_start;
    v3 count = rng / cube_width;

    // Just for debugging purposes
    if (ggfx->units_debug.selected_manipulator == u.manipulator) {
        debug_.cube_center = v3(u.position);
        debug_.cube_scale = 2.0f*scale;
        debug_.lod_level = level_lod;
        debug_.screen_space_level = level_screen_space;
        debug_.final_level = level;
        debug_.cube_start = cube_start;
        debug_.cube_end = cube_end;
        debug_.count = count;
    }
}

void update_sdf_octree() {
    for (int i = 0; i < ggfx->units_info.unit_count; ++i) {
        sdf_unit &u = ggfx->units_arrays.units[i];
        add_sdf_unit(u);
    }
}
