#include "texture.hpp"
#include "log.hpp"
#include "render_context.hpp"
#include "vulkan/vulkan_core.h"

static constexpr VkImageUsageFlags default_image_usage_flags_ = 
    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | 
    VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;

texture::texture() 
: image_(VK_NULL_HANDLE), image_view_(VK_NULL_HANDLE), descriptor_set_{ VK_NULL_HANDLE }, last_layout_(VK_IMAGE_LAYOUT_UNDEFINED),
    last_used_(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT), last_used_type_(texture_descriptor_type::max_enum) {

}

texture::texture(VkImage image, VkImageView image_view, bool is_depth) 
: image_(image), image_view_(image_view), last_used_(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT), is_depth_(is_depth),
    last_used_type_(texture_descriptor_type::max_enum), last_layout_(VK_IMAGE_LAYOUT_UNDEFINED) {
    // Create descriptor sets
    for (u32 i = 0; i < (u32)texture_descriptor_type::max_enum; ++i) {
        VkDescriptorSetLayout layout = get_descriptor_set_layout(
            convert_descriptor_type_((texture_descriptor_type)i), 1);

        VkDescriptorSetAllocateInfo allocate_info = {};
        allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocate_info.descriptorPool = gctx->descriptor_pool;
        allocate_info.descriptorSetCount = 1;
        allocate_info.pSetLayouts = &layout;

        vkAllocateDescriptorSets(
            gctx->device, &allocate_info, &descriptor_set_[i]);

        VkDescriptorImageInfo image_info = {};
        VkWriteDescriptorSet write = {};

        image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        image_info.imageView = image_view_;
        image_info.sampler = VK_NULL_HANDLE;

        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = descriptor_set_[i];
        write.dstBinding = 0;
        write.dstArrayElement = 0;
        write.descriptorCount = 1;
        write.descriptorType = convert_descriptor_type_((texture_descriptor_type)i);
        write.pImageInfo = &image_info;

        vkUpdateDescriptorSets(gctx->device, 1, &write, 0, nullptr);
    }
} 

texture::texture(VkImage image, VkImageView image_view, texture_descriptor_type type, bool is_depth) 
: image_(image), image_view_(image_view), last_used_(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT), is_depth_(is_depth),
    last_used_type_(texture_descriptor_type::max_enum), last_layout_(VK_IMAGE_LAYOUT_UNDEFINED) {
    VkDescriptorSetLayout layout = get_descriptor_set_layout(
        convert_descriptor_type_((texture_descriptor_type)type), 1);

    VkDescriptorSetAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocate_info.descriptorPool = gctx->descriptor_pool;
    allocate_info.descriptorSetCount = 1;
    allocate_info.pSetLayouts = &layout;

    vkAllocateDescriptorSets(
        gctx->device, &allocate_info, &descriptor_set_[(u32)type]);

    VkDescriptorImageInfo image_info = {};
    VkWriteDescriptorSet write = {};

    image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    image_info.imageView = image_view_;
    image_info.sampler = VK_NULL_HANDLE;

    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = descriptor_set_[(u32)type];
    write.dstBinding = 0;
    write.dstArrayElement = 0;
    write.descriptorCount = 1;
    write.descriptorType = convert_descriptor_type_(type);
    write.pImageInfo = &image_info;

    vkUpdateDescriptorSets(gctx->device, 1, &write, 0, nullptr);
}

texture &texture::operator=(const texture &other) {
    image_ = other.image_;
    image_view_ = other.image_view_;
    memcpy(descriptor_set_, other.descriptor_set_, sizeof(VkDescriptorSet) * (u32)texture_descriptor_type::max_enum);

    return *this;
}

VkDescriptorSet texture::get_descriptor_set(VkDescriptorType type) {
    return descriptor_set_[(u32)convert_descriptor_type_vk_(type)];
}

u32 texture::convert_descriptor_type_vk_(VkDescriptorType type) {
    switch (type) {
    case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE: return (u32)texture_descriptor_type::sampled_image;
    case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: return (u32)texture_descriptor_type::storage_image;
    case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: return (u32)texture_descriptor_type::input_attachment;
    default: return (u32)texture_descriptor_type::max_enum;
    }
}

VkDescriptorType texture::convert_descriptor_type_(texture_descriptor_type type) {
    switch (type) {
    case texture_descriptor_type::sampled_image: return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    case texture_descriptor_type::storage_image: return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    case texture_descriptor_type::input_attachment: return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    default: return VK_DESCRIPTOR_TYPE_MAX_ENUM;
    }
}

VkDescriptorSet *texture::get_descriptor_sets() {
    return descriptor_set_;
}

void texture::transition_layout(render_graph &graph, VkImageLayout dst, VkPipelineStageFlags stage) {
    VkImageMemoryBarrier image_barrier = {};
    image_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_barrier.srcAccessMask = find_access_flags_for_layout(last_layout_);
    image_barrier.oldLayout = last_layout_;
    image_barrier.image = image_;

    image_barrier.newLayout = dst;
    image_barrier.dstAccessMask = find_access_flags_for_layout(image_barrier.newLayout);

    image_barrier.subresourceRange.aspectMask = is_depth_ ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    image_barrier.subresourceRange.baseMipLevel = 0;
    // For now only support this (will add super soon)
    image_barrier.subresourceRange.levelCount = 1;
    image_barrier.subresourceRange.baseArrayLayer = 0;
    image_barrier.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(graph.command_buffer_, last_used_, 
        stage, 0, 0, nullptr, 0, nullptr, 1, &image_barrier);

    last_used_ = stage;
    last_layout_ = image_barrier.newLayout;
}

void texture::prepare_compute_resource_(render_graph &graph, VkDescriptorType type, void *raw_ptr) {
    texture *ptr = (texture *)raw_ptr;

    texture_descriptor_type converted_type = (texture_descriptor_type)convert_descriptor_type_vk_(type);

    VkImageMemoryBarrier image_barrier = {};
    image_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_barrier.srcAccessMask = find_access_flags_for_layout(ptr->last_layout_);
    image_barrier.oldLayout = ptr->last_layout_;
    image_barrier.image = ptr->image_;

    switch (converted_type) {
    case texture_descriptor_type::sampled_image: {
        image_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    } break;

    case texture_descriptor_type::storage_image: {
        image_barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    } break;

    case texture_descriptor_type::input_attachment: {
        image_barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    } break;

    default: { panic_and_exit(); };
    }

    image_barrier.dstAccessMask = find_access_flags_for_layout(image_barrier.newLayout);

    image_barrier.subresourceRange.aspectMask = ptr->is_depth_ ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    image_barrier.subresourceRange.baseMipLevel = 0;
    // For now only support this (will add super soon)
    image_barrier.subresourceRange.levelCount = 1;
    image_barrier.subresourceRange.baseArrayLayer = 0;
    image_barrier.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(graph.command_buffer_, ptr->last_used_, 
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &image_barrier);

    ptr->last_used_ = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    ptr->last_used_type_ = converted_type;
    ptr->last_layout_ = image_barrier.newLayout;
}
