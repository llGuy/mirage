#include "log.hpp"
#include "buffer.hpp"
#include "render_context.hpp"
#include "vulkan/vulkan_core.h"

gpu_buffer::gpu_buffer() 
: last_used_(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT) {

}

gpu_buffer::gpu_buffer(VkBuffer buf, u32 size, VkBufferUsageFlags usage) 
: buffer_(buf), size_(size), last_used_(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT) {
    VkDescriptorType descriptor_type = VK_DESCRIPTOR_TYPE_MAX_ENUM;
    if (usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) {
        descriptor_type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    }
    else if (usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) {
        descriptor_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    }

    buffer_descriptor_type type = (buffer_descriptor_type)convert_descriptor_type_vk_(descriptor_type);

    VkDescriptorSetLayout layout = get_descriptor_set_layout(descriptor_type, 1);

    VkDescriptorSetAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocate_info.descriptorPool = gctx->descriptor_pool;
    allocate_info.descriptorSetCount = 1;
    allocate_info.pSetLayouts = &layout;

    vkAllocateDescriptorSets(gctx->device, &allocate_info, &descriptor_set_[(u32)type]);

    VkDescriptorBufferInfo buffer_info = {};
    VkWriteDescriptorSet write = {};

    buffer_info.buffer = buffer_;
    buffer_info.offset = 0;
    buffer_info.range = size_;

    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = descriptor_set_[(u32)type];
    write.dstBinding = 0;
    write.dstArrayElement = 0;
    write.descriptorCount = 1;
    write.descriptorType = descriptor_type;
    write.pBufferInfo = &buffer_info;

    vkUpdateDescriptorSets(gctx->device, 1, &write, 0, nullptr);
}

void gpu_buffer::update(render_graph &graph, u32 offset, u32 size, void *data) {
    VkBufferMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barrier.size = size;
    barrier.offset = offset;
    barrier.buffer = buffer_;
    barrier.srcAccessMask = find_access_flags_for_stage(last_used_);
    barrier.dstAccessMask = find_access_flags_for_stage(VK_PIPELINE_STAGE_TRANSFER_BIT);

    vkCmdPipelineBarrier(graph.command_buffer_, last_used_, VK_PIPELINE_STAGE_TRANSFER_BIT, 
        0, 0, nullptr, 1, &barrier, 0, nullptr);

    vkCmdUpdateBuffer(graph.command_buffer_, buffer_, offset, size, data);

    last_used_ = VK_PIPELINE_STAGE_TRANSFER_BIT;
}

u32 gpu_buffer::convert_descriptor_type_vk_(VkDescriptorType type) {
    switch (type) {
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: return (u32)buffer_descriptor_type::uniform_buffer;
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER: return (u32)buffer_descriptor_type::storage_buffer;
        default: panic_and_exit(); return 0;
    }
}

VkDescriptorType gpu_buffer::convert_descriptor_type_(buffer_descriptor_type type) {
    switch (type) {
        case buffer_descriptor_type::storage_buffer: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        case buffer_descriptor_type::uniform_buffer: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        default: panic_and_exit(); return VK_DESCRIPTOR_TYPE_MAX_ENUM;
    }
}

VkDescriptorSet gpu_buffer::get_descriptor_set(VkDescriptorType type) {
    return descriptor_set_[(u32)convert_descriptor_type_vk_(type)];
}

void gpu_buffer::prepare_compute_resource_(render_graph &graph, VkDescriptorType type, void *raw_ptr) {
    gpu_buffer *ptr = (gpu_buffer *)raw_ptr;

    VkBufferMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barrier.size = ptr->size_;
    barrier.offset = 0;
    barrier.buffer = ptr->buffer_;
    barrier.srcAccessMask = find_access_flags_for_stage(ptr->last_used_);
    
    switch (type) {
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: {
            barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        } break;

        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER: {
            barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
        } break;

        default: panic_and_exit(); return;
    }

    // Issue necessary memory barriers
    vkCmdPipelineBarrier(graph.command_buffer_, ptr->last_used_, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0, 0, nullptr, 1, &barrier, 0, nullptr);
}

VkDescriptorSet *gpu_buffer::get_descriptor_sets() {
    return descriptor_set_;
}

gpu_buffer make_uniform_buffer(u32 size) {
    VkBuffer buf;

    VkBufferUsageFlags usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    VkBufferCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size = size;
    info.usage = usage;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vkCreateBuffer(gctx->device, &info, nullptr, &buf);

    // Just make it device local
    allocate_buffer_memory(buf, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    return gpu_buffer(buf, size, usage);
}

gpu_buffer make_storage_buffer(u32 size) {
    VkBuffer buf;

    VkBufferUsageFlags usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    VkBufferCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size = size;
    info.usage = usage;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vkCreateBuffer(gctx->device, &info, nullptr, &buf);

    // Just make it device local
    allocate_buffer_memory(buf, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    return gpu_buffer(buf, size, usage);
}
