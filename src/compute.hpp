#pragma once

#include "file.hpp"
#include <type_traits>
#include "uniform.hpp"
#include "heap_array.hpp"
#include "render_graph.hpp"
#include <vulkan/vulkan.hpp>
#include "render_context.hpp"
#include "vulkan/vulkan_core.h"

// Use this type for templated functions which use type to specify push constant
struct no_push_constant {};

class compute_pass {
public:
    compute_pass() = default;

    compute_pass(const char *src_path, u32 push_constant_size, const buffer<uprototype> &uniforms);

    // This 
    template <typename PK, typename ...T>
    void bind_resources(render_graph &graph, const PK *push_constant, T &...resources) {
        if constexpr (!std::is_same<no_push_constant, PK>::value) {
            vkCmdPushConstants(graph.command_buffer_, layout_, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PK), push_constant);
        }

        VkDescriptorSet *uniform_descriptor_sets[] = { resources.get_descriptor_sets()... };
        
        // This is a bit ugly but we'll keep for now
        using conversion_proc = u32(*)(VkDescriptorType);
        conversion_proc convert_procs[] = { &std::remove_reference<decltype(resources)>::type::convert_descriptor_type_vk_... };

        using prepare_proc = void(*)(render_graph &graph, VkDescriptorType, void *);
        prepare_proc prepare_procs[] = { &std::remove_reference<decltype(resources)>::type::prepare_compute_resource_... };

        void *resources_raw_ptr[] = { (void *)&resources... };

        // Fill this array
        VkDescriptorSet descriptor_sets[sizeof...(T)];
        for (int i = 0; i < sizeof...(T); ++i) {
            descriptor_sets[i] = uniform_descriptor_sets[i][convert_procs[i](descriptor_types_[i])];
            prepare_procs[i](graph, descriptor_types_[i], resources_raw_ptr[i]);
        }

        vkCmdBindDescriptorSets(graph.command_buffer_, VK_PIPELINE_BIND_POINT_COMPUTE, layout_, 0, sizeof...(T), descriptor_sets, 0, nullptr);
    }

    void run(render_graph &graph, u32 count_x, u32 count_y, u32 count_z) {
        vkCmdBindPipeline(graph.command_buffer_, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_);
        vkCmdDispatch(graph.command_buffer_, count_x, count_y, count_z);
    }

    // void run(render_graph &graph, iv3)

private:
    std::string make_shader_src_path(const char *path) const;

private:
    VkPipeline pipeline_;
    VkPipelineLayout layout_;
    heap_array<VkDescriptorType> descriptor_types_;
};

template <typename ...UP>
compute_pass make_compute_pass(const char *src_path, u32 push_constant_size, UP ...up) {
    uprototype uniforms[] = {up...};
    buffer<uprototype> prototypes { uniforms, sizeof...(UP) };
    return compute_pass(src_path, push_constant_size, prototypes);
}

template <typename PK, typename ...UP>
compute_pass make_compute_pass(const char *src_path, UP ...up) {
    u32 push_constant_size = std::is_same<no_push_constant, PK>::value ? 0 : sizeof(PK);

    uprototype uniforms[] = {up...};
    buffer<uprototype> prototypes { uniforms, sizeof...(UP) };
    return compute_pass(src_path, push_constant_size, prototypes);
}
