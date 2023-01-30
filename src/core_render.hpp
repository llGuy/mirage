#pragma once

#include "blob.hpp"
#include "types.hpp"
#include "buffer.hpp"
#include "texture.hpp"
#include "render_graph.hpp"

#include <vulkan/vulkan.h>

// All renderer resources (textures, uniforms, pipelines, etc...)
extern struct graphics_resources {
    // Textures, buffers, etc...
    heap_array<texture> swapchain_targets;
    gpu_buffer time_uniform_data;
    // All blobs will be stored here one after the other without spatial organization
    // TODO: Add octree structure to better organize them and make iteration more efficient
    gpu_buffer blob_data;
    blob_array *blobs;
} *ggfx;

void init_core_render();
void run_render();

// All rendering functionality
void init_final_pass();
void run_final_pass(render_graph &, texture &target);
