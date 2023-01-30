#pragma once

#include <vulkan/vulkan.h>

// TODO: Make this a proper render graph (actually no need, make all resources track their own stuff)
class render_graph {
public:
    // Already allocated command buffer
    render_graph(VkCommandBuffer command_buffer);

    void submit(VkQueue queue, VkSemaphore to_wait, VkSemaphore to_signal, 
        VkPipelineStageFlags stage, VkFence fence);

private:
    VkCommandBuffer command_buffer_;

    // TODO: Resource tracking for barriers etc...

    friend class compute_pass;
    friend class texture;
    friend class gpu_buffer;
};
