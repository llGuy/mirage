#include "compute.hpp"
#include <filesystem>

compute_pass::compute_pass(const char *src_path, u32 push_constant_size, const buffer<uprototype> &uniforms) 
: descriptor_types_(uniforms.size) {
    // Push constant
    VkPushConstantRange push_constant_range = {};
    push_constant_range.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;;
    push_constant_range.offset = 0;
    push_constant_range.size = push_constant_size;

    // Pipeline layout
    VkDescriptorSetLayout *layouts = stack_alloc(VkDescriptorSetLayout, uniforms.size);
    for (u32 i = 0; i < uniforms.size; ++i) {
        layouts[i] = get_descriptor_set_layout(uniforms[i].type, uniforms[i].count ? uniforms[i].count : 1);
        descriptor_types_[i] = uniforms[i].type;
    }

    VkPipelineLayoutCreateInfo pipeline_layout_info = {};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = uniforms.size;
    pipeline_layout_info.pSetLayouts = layouts;

    if (push_constant_size) {
        pipeline_layout_info.pushConstantRangeCount = 1;
        pipeline_layout_info.pPushConstantRanges = &push_constant_range;
    }

    VK_CHECK(vkCreatePipelineLayout(gctx->device, &pipeline_layout_info, nullptr, &layout_));

    // Shader stage
    heap_array<u8> src_bytes = file(
        make_shader_src_path(src_path), 
        file_type_bin | file_type_in).read_binary();

    VkShaderModule shader_module;
    VkShaderModuleCreateInfo shader_info = {};
    shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_info.codeSize = src_bytes.size();
    shader_info.pCode = (u32 *)src_bytes.data();

    VK_CHECK(vkCreateShaderModule(gctx->device, &shader_info, NULL, &shader_module));

    VkPipelineShaderStageCreateInfo module_info = {};
    module_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    module_info.pName = "main";
    module_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    module_info.module = shader_module;

    VkComputePipelineCreateInfo compute_pipeline_info = {};
    compute_pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    compute_pipeline_info.stage = module_info;
    compute_pipeline_info.layout = layout_;

    VK_CHECK(vkCreateComputePipelines(gctx->device, VK_NULL_HANDLE, 1, &compute_pipeline_info, nullptr, &pipeline_));
}

std::string compute_pass::make_shader_src_path(const char *path) const {
    std::string str_path = path;
    str_path = std::string(MIRAGE_PROJECT_ROOT) +
        (char)std::filesystem::path::preferred_separator +
        std::string("res") + 
        (char)std::filesystem::path::preferred_separator +
        std::string("spv") + 
        (char)std::filesystem::path::preferred_separator +
        str_path +
        ".comp.spv";
    return str_path;
}
