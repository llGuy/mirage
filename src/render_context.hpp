#pragma once

#include "log.hpp"
#include "types.hpp"
#include "heap_array.hpp"
#include "descriptor_helper.hpp"

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#define VK_CHECK(call) \
  if (call != VK_SUCCESS) { log_error("%s failed\n", #call); panic_and_exit(); }

// Global render context
extern struct render_context {
    // Various flags
    u32 is_validation_enabled : 1;

    // Instance
    VkInstance instance;
    heap_array<const char *> layers;

    // Device
    VkPhysicalDevice gpu;
    VkDevice device;
    s32 graphics_family, present_family;
    VkQueue graphics_queue, present_queue;

    // Window / Surface
    GLFWwindow *window;
    u32 window_width, window_height;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    VkExtent2D swapchain_extent;
    VkFormat swapchain_format;
    heap_array<VkImage> images;
    heap_array<VkImageView> image_views;

    // Debug capabilities
    VkDebugUtilsMessengerEXT messenger;

    // Other shit
    VkCommandPool command_pool;
    VkDescriptorPool descriptor_pool;
    descriptor_set_layout_category layout_categories[descriptor_set_layout_category::category_count];

    // Debug overlay
    VkRenderPass imgui_render_pass;
} *gctx;

void init_render_context();
bool is_running();
void poll_input();
u32 acquire_next_swapchain_image(VkSemaphore);
void present_swapchain_image(VkSemaphore to_wait, u32 image_idx);

// Helpers for descriptor layouts
VkDescriptorSetLayout get_descriptor_set_layout(VkDescriptorType type, u32 count);

// Helpers for synchronization
VkAccessFlags find_access_flags_for_stage(VkPipelineStageFlags stage);
VkAccessFlags find_access_flags_for_layout(VkImageLayout layout);

// Helpers for memory management
u32 find_memory_type(VkMemoryPropertyFlags properties, VkMemoryRequirements &memory_requirements);
VkDeviceMemory allocate_buffer_memory(VkBuffer buffer, VkMemoryPropertyFlags properties);
VkDeviceMemory allocate_image_memory(VkImage image, VkMemoryPropertyFlags properties, u32 *size);

extern PFN_vkDebugMarkerSetObjectTagEXT vkDebugMarkerSetObjectTag;
extern PFN_vkDebugMarkerSetObjectNameEXT vkDebugMarkerSetObjectName;
extern PFN_vkCmdDebugMarkerBeginEXT vkCmdDebugMarkerBegin;
extern PFN_vkCmdDebugMarkerEndEXT vkCmdDebugMarkerEnd;
extern PFN_vkCmdDebugMarkerInsertEXT vkCmdDebugMarkerInsert;
