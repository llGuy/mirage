#pragma once

#include "string.hpp"
#include <vulkan/vulkan.h>

// TODO: Make this a proper render graph (actually no need, make all resources track their own stuff)
class render_graph {
public:
    enum flags { none = 0, one_time = 1 };

    // Already allocated command buffer
    render_graph(VkCommandBuffer command_buffer, flags one_time = none);

    void submit(VkQueue queue, VkSemaphore to_wait, VkSemaphore to_signal, 
        VkPipelineStageFlags stage, VkFence fence);

    inline VkCommandBuffer cmdbuf() const { return command_buffer_; }

public:
    static uint32_t res_name_id_counter;

private:
    VkCommandBuffer command_buffer_;

    // TODO: Resource tracking for barriers etc...

    friend class compute_pass;
    friend class texture;
    friend class gpu_buffer;
};

#define rdg(x) (get_id<crc32<sizeof(x) - 2>(x) ^ 0xFFFFFFFF>(render_graph::res_name_id_counter))
