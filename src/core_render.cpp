#include "file.hpp"
#include "render_graph.hpp"
#include "time.hpp"
#include "memory.hpp"
#include "core_render.hpp"
#include "render_context.hpp"
#include "vulkan/vulkan_core.h"

graphics_resources *ggfx;
VkPipeline pso;
VkPipelineLayout pso_layout;

void init_core_render() {
    ggfx = mem_alloc<graphics_resources>();

    ggfx->graph.setup_swapchain({
        .swapchain_image_count = gctx->images.size(),
        .images = gctx->images.data(),
        .image_views = gctx->image_views.data(),
        .extent = gctx->swapchain_extent
    });

    { // Create pipeline state object
        VkPipelineRenderingCreateInfo rendering_info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &gctx->swapchain_format,
        };

        auto vertex_src_bytes = file(make_shader_src_path("triangle", VK_SHADER_STAGE_VERTEX_BIT), file_type_bin | file_type_in).read_binary();
        auto fragment_src_bytes = file(make_shader_src_path("triangle", VK_SHADER_STAGE_FRAGMENT_BIT), file_type_bin | file_type_in).read_binary();

        VkShaderModuleCreateInfo vertex_module_info = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = vertex_src_bytes.size(),
            .pCode = (u32 *)vertex_src_bytes.data()
        };

        VkShaderModuleCreateInfo fragment_module_info = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = fragment_src_bytes.size(),
            .pCode = (u32 *)fragment_src_bytes.data()
        };

        VkShaderModule vertex_module, fragment_module;
        vkCreateShaderModule(gctx->device, &vertex_module_info, nullptr, &vertex_module);
        vkCreateShaderModule(gctx->device, &fragment_module_info, nullptr, &fragment_module);

        VkPipelineShaderStageCreateInfo vertex = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pName = "main",
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertex_module
        };

        VkPipelineShaderStageCreateInfo fragment = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pName = "main",
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fragment_module
        };

        VkPipelineShaderStageCreateInfo stages[] = { vertex, fragment };

        VkPipelineVertexInputStateCreateInfo vertex_input = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        };

        VkPipelineInputAssemblyStateCreateInfo input_assembly = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
        };

        VkViewport res = {.width = 1, .height = 1, .maxDepth = 1.0f};
        VkRect2D rect = {.extent = {1, 1}};

        VkPipelineViewportStateCreateInfo viewport = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .pViewports = &res,
            .scissorCount = 1,
            .pScissors = &rect
        };

        VkPipelineRasterizationStateCreateInfo rasterization = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_NONE,
            .frontFace = VK_FRONT_FACE_CLOCKWISE,
            .lineWidth = 1.0f
        };

        VkPipelineMultisampleStateCreateInfo multisample = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
            .minSampleShading = 1.0f
        };

        VkPipelineDepthStencilStateCreateInfo depth = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable = VK_FALSE,
            .depthWriteEnable = VK_FALSE
        };

        VkDynamicState dynamic_states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

        VkPipelineDynamicStateCreateInfo dynamic_state = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = 2,
            .pDynamicStates = dynamic_states
        };

        VkPipelineColorBlendAttachmentState attachment = {
            .blendEnable = VK_FALSE,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
        };

        VkPipelineColorBlendStateCreateInfo blend = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .attachmentCount = 1,
            .pAttachments = &attachment,
            .logicOpEnable = VK_FALSE,
            .logicOp = VK_LOGIC_OP_COPY
        };

        VkPipelineLayoutCreateInfo layout_info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 0,
            .pushConstantRangeCount = 0
        };

        vkCreatePipelineLayout(gctx->device, &layout_info, nullptr, &pso_layout);

        VkGraphicsPipelineCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = &rendering_info,
            .stageCount = 2,
            .pStages = stages,
            .pVertexInputState = &vertex_input,
            .pInputAssemblyState = &input_assembly,
            .pViewportState = &viewport,
            .pRasterizationState = &rasterization,
            .pMultisampleState = &multisample,
            .pDepthStencilState = &depth,
            .pColorBlendState = &blend,
            .pDynamicState = &dynamic_state,
            .layout = pso_layout,
            .renderPass = VK_NULL_HANDLE
        };

        vkCreateGraphicsPipelines(gctx->device, VK_NULL_HANDLE, 1, &info, nullptr, &pso);
    }
}

void run_render() {
    poll_input();

    render_graph *graph = &ggfx->graph;

    // Starts a series of commands
    graph->begin();

    { // Example setup a compute pass (all in immediate)
        compute_pass &pass = graph->add_compute_pass(STG("compute-example"));
        pass.set_source("basic"); // Name of the file (can use custom filename resolution
        pass.send_data(gtime->current_time); // Can send any arbitrary data
        pass.add_storage_image(RES("compute-target")); // Sets the render target
        pass.dispatch_waves(16, 16, 1, RES("compute-target"));
    }

    { // Example of a render pass
        render_pass &pass = graph->add_render_pass(STG("raster-example"));
        pass.add_color_attachment(RES("compute-target")); // Don't clear target
        pass.draw_commands([] (VkCommandBuffer cmdbuf, VkRect2D rect, void *data) {
            VkPipeline pso = *(VkPipeline *)data;

            vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pso);

            VkViewport viewport = { 
                .width = (float)rect.extent.width, .height = (float)rect.extent.height, .maxDepth = 1.0f, .minDepth = 0.0f
            };

            vkCmdSetViewport(cmdbuf, 0, 1, &viewport);
            vkCmdSetScissor(cmdbuf, 0, 1, &rect);

            vkCmdDraw(cmdbuf, 3, 1, 0, 0);
        }, pso);
    }

    // Present to the screen
    graph->present(RES("compute-target"));

    // Finishes a series of commands
    graph->end();
}
