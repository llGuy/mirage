#include "time.hpp"
#include "memory.hpp"
#include "core_render.hpp"
#include "render_context.hpp"

graphics_resources *ggfx;
uint32_t gres_name_id_counter;

// Synchronisation stuff
static heap_array<VkSemaphore> image_ready_semaphores_;
static heap_array<VkSemaphore> render_finished_semaphores_;
static heap_array<VkFence> fences_;

// Frame tracker
static u32 current_frame_;
static u32 max_frames_in_flight_;

// Command buffers
static heap_array<VkCommandBuffer> command_buffers_;

void init_core_render() {
    ggfx = mem_alloc<graphics_resources>();

    gres_name_id_counter = 0;

    // Synchronisation
    max_frames_in_flight_ = 2;
    image_ready_semaphores_ = heap_array<VkSemaphore>(max_frames_in_flight_);
    render_finished_semaphores_ = heap_array<VkSemaphore>(max_frames_in_flight_);
    fences_ = heap_array<VkFence>(max_frames_in_flight_);

    VkSemaphoreCreateInfo semaphore_info = {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (u32 i = 0; i < max_frames_in_flight_; ++i) {
        vkCreateSemaphore(gctx->device, &semaphore_info, nullptr, &image_ready_semaphores_[i]);
        vkCreateSemaphore(gctx->device, &semaphore_info, nullptr, &render_finished_semaphores_[i]);
        vkCreateFence(gctx->device, &fence_info, nullptr, &fences_[i]);
    }

    // Command buffers
    command_buffers_ = heap_array<VkCommandBuffer>(gctx->images.size());
    VkCommandBufferAllocateInfo command_buffer_info = {};
    command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_info.commandBufferCount = command_buffers_.size();
    command_buffer_info.commandPool = gctx->command_pool;
    command_buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    vkAllocateCommandBuffers(gctx->device, &command_buffer_info, command_buffers_.data());
}

#define RENDER_GRAPH_IMPL

void run_render() {
    poll_input();

#ifdef RENDER_GRAPH_IMPL
    render_graph *graph = &ggfx->graph;

    // Starts a series of commands
    graph->begin();

    { // Example setup a compute pass (all in immediate)
        compute_pass &pass = graph->add_compute_pass(STG("compute-example"));
        pass.set_source("basic"); // Name of the file (can use custom filename resolution
        pass.send_data(gtime->current_time); // Can send any arbitrary data
        pass.add_storage_image(RES("compute-target")); // Sets the render target
        pass.dispatch_waves(16, 16, 1);
    }

    // Present to the screen
    graph->present(RES("compute-target"));

    // Finishes a series of commands
    graph->end();

#else
    // Get swapchain image
    u32 swapchain_image_idx = acquire_next_swapchain_image(image_ready_semaphores_[current_frame_]);
    vkWaitForFences(gctx->device, 1, &fences_[current_frame_], true, UINT64_MAX);
    vkResetFences(gctx->device, 1, &fences_[current_frame_]);

    VkCommandBuffer current_command_buffer = command_buffers_[swapchain_image_idx];

    ggfx->swapchain_targets[swapchain_image_idx].transition_layout(graph, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);

    // Submit command buffer
    graph.submit(gctx->graphics_queue, image_ready_semaphores_[current_frame_], 
        render_finished_semaphores_[current_frame_], VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 
        fences_[current_frame_]);

    // Present to screen
    present_swapchain_image(render_finished_semaphores_[current_frame_], swapchain_image_idx);
    current_frame_ = (current_frame_ + 1) % max_frames_in_flight_;
#endif
}
