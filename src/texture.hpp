#pragma once

#include "uniform.hpp"
#include "heap_array.hpp"
#include "render_graph.hpp"

#include <vulkan/vulkan.h>

enum class texture_descriptor_type : u32 {
    sampled_image, storage_image, input_attachment, max_enum
};

class texture : public uobject {
public:
    // Construction
    texture();
    texture(VkImage image, VkImageView image_view, bool is_depth = false);
    texture(VkImage image, VkImageView image_view, texture_descriptor_type type, bool is_depth = false);

    texture &operator=(const texture &other);

    // Descriptor set
    VkDescriptorSet get_descriptor_set(VkDescriptorType type) override;

    void transition_layout(render_graph &graph, VkImageLayout dst, VkPipelineStageFlags stage);

private:
    static u32 convert_descriptor_type_vk_(VkDescriptorType type);
    static VkDescriptorType convert_descriptor_type_(texture_descriptor_type type);
    // Prepares a texture
    static void prepare_compute_resource_(render_graph &graph, VkDescriptorType type, void *);

    // This returns all the descriptor sets
    VkDescriptorSet *get_descriptor_sets();

private:
    VkImage image_;
    VkImageView image_view_;

    // Table - these will always be created for all images
    VkDescriptorSet descriptor_set_[(u32)texture_descriptor_type::max_enum];

    VkPipelineStageFlags last_used_;
    texture_descriptor_type last_used_type_;
    VkImageLayout last_layout_;

    bool is_depth_;

    friend class compute_pass;
};
