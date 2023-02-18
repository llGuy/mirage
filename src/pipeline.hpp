#pragma once

#include "heap_array.hpp"

#include <vulkan/vulkan.h>

#if 0
class pso_shader {
public:
    pso_shader(const char *path);

private:
    VkShaderModule module_;
    VkShaderStageFlags stage_;
};

class pso_config {
public:
    template <typename ...T>
    pso_config(T ...shaders)
    : input_assembly_{},
      vertex_input_{},
      viewport_{},
      rasterization_{},
      multisample_{},
      blending_{},
      dynamic_state_{},
      depth_stencil_{},
      layouts_{},
      shader_stages_(sizeof...(shaders)),
      push_constant_size_(0),
      viewport_info_{},
      create_info_{} {
        shader_stages_.zero();
        set_shader_stages_(
            makeArray<VulkanShader, AllocationType::Linear>(shaders...));

        set_default_values_();
    }

    void enable_blending_same(
        uint32_t attachment_idx,
        VkBlendOp op, VkBlendFactor src, VkBlendFactor dst);

    void enable_depth_testing();

    template <typename ...T>
    void configure_layouts(
        size_t push_constant_size,
        T ...layouts) {
        push_constant_size_ = push_constant_size;
        layouts_ =
            heap_array<VkDescriptorSetLayout>(layouts...);
    }

    void configure_vertex_input(uint32_t attrib_count, uint32_t binding_count);

    void set_binding(
        uint32_t binding, uint32_t stride, VkVertexInputRate input_rate);

    void set_binding_attribute(
        uint32_t location, uint32_t binding, VkFormat format, uint32_t offset);

    void set_topology(VkPrimitiveTopology topology);
    void set_to_wireframe();

private:
    void set_default_values_();

    void set_shader_stages_(
        const Array<VulkanShader, AllocationType::Linear> &sources);

    void finish_configuration_(
        VulkanDescriptorSetLayoutMaker &layout);

private:
    VkPipelineInputAssemblyStateCreateInfo input_assembly_;
    VkPipelineVertexInputStateCreateInfo vertex_input_;
    heap_array<VkVertexInputAttributeDescription> attributes_;
    heap_array<VkVertexInputBindingDescription> bindings_;
    VkViewport viewport_;
    VkRect2D rect_;
    VkPipelineViewportStateCreateInfo viewport_info_;
    VkPipelineRasterizationStateCreateInfo rasterization_;
    VkPipelineMultisampleStateCreateInfo multisample_;
    VkPipelineColorBlendStateCreateInfo blending_;
    heap_array<VkPipelineColorBlendAttachmentState> blend_states_;
    VkPipelineDynamicStateCreateInfo dynamic_state_;
    heap_array<VkDynamicState> dynamic_states_;
    VkPipelineDepthStencilStateCreateInfo depth_stencil_;
    heap_array<VkDescriptorSetLayout> layouts_;
    heap_array<VkPipelineShaderStageCreateInfo> shader_stages_;
    size_t push_constant_size_;

    VkGraphicsPipelineCreateInfo create_info_;

    VkPipelineLayout pso_layout_;
};
#endif
