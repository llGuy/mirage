#pragma once

#include "uniform.hpp"
#include "render_graph.hpp"

enum class buffer_descriptor_type : u32 {
    uniform_buffer, storage_buffer, max_enum
};

class gpu_buffer : public uobject {
public:
    gpu_buffer();
    gpu_buffer(VkBuffer buf, u32 size, VkBufferUsageFlags usage);

    void update(render_graph &, u32 offset, u32 size, void *data);

    VkDescriptorSet get_descriptor_set(VkDescriptorType type) override;

private:
    static u32 convert_descriptor_type_vk_(VkDescriptorType type);
    static VkDescriptorType convert_descriptor_type_(buffer_descriptor_type type);

    static void prepare_compute_resource_(render_graph &graph, VkDescriptorType type, void *);

    VkDescriptorSet *get_descriptor_sets();

private:
    VkBuffer buffer_;
    u32 size_;
    VkDeviceMemory memory_;
    VkPipelineStageFlags last_used_;
    VkAccessFlags last_access_;

    VkDescriptorSet descriptor_set_[(u32)buffer_descriptor_type::max_enum];

    friend class compute_pass;
};

gpu_buffer make_uniform_buffer(u32 size);
