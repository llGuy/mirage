#include "render_graph.hpp"
#include "vulkan/vulkan_core.h"

render_graph::render_graph(VkCommandBuffer command_buffer, flags one_time) 
: command_buffer_(command_buffer) {
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.pInheritanceInfo = nullptr;
    begin_info.flags = one_time ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : 0;
    vkBeginCommandBuffer(command_buffer, &begin_info);
}

void render_graph::submit(VkQueue queue, VkSemaphore to_wait, VkSemaphore to_signal, 
    VkPipelineStageFlags stage, VkFence fence) {
    vkEndCommandBuffer(command_buffer_);

    VkSubmitInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    info.commandBufferCount = 1;
    info.pCommandBuffers = &command_buffer_;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &to_wait;
    info.signalSemaphoreCount = 1;
    info.pSignalSemaphores = &to_signal;
    info.pWaitDstStageMask = &stage;
    vkQueueSubmit(queue, 1, &info, fence);
}
