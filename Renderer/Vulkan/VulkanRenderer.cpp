#include "VulkanRenderer.h"

u32 FindMemoryType(u32 type_bits, VkMemoryPropertyFlags flags) {
    for (u32 i = 0; i < VulkanPhysicalDevice::memory_properties.memoryTypeCount; ++i) {
        if ((type_bits & (1 << i)) && (VulkanPhysicalDevice::memory_properties.memoryTypes[i].propertyFlags & flags) == flags) {
            return i;
        }
    }

    LogFatal("Failed to find suitable memory type");
    return 0;
}


VkSemaphore CreateSemaphore(VkSemaphoreCreateFlags flags) {
    VkSemaphoreCreateInfo semaphore_info = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    semaphore_info.flags = flags;

    VkSemaphore handle;
    VK_CHECK(vkCreateSemaphore(VulkanDevice::handle, &semaphore_info, 0, &handle));
    return handle;
}

void DestroySemaphore(VkSemaphore handle) {
    vkDestroySemaphore(VulkanDevice::handle, handle, 0);
}

VkFence CreateFence(VkFenceCreateFlags flags) {
    VkFenceCreateInfo fence_info = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fence_info.flags = flags;

    VkFence handle;
    VK_CHECK(vkCreateFence(VulkanDevice::handle, &fence_info, 0, &handle));
    return handle;
}

void DestroyFence(VkFence handle) {
    vkDestroyFence(VulkanDevice::handle, handle, 0);
}

VkQueryPool RenderStats::query_pool = VK_NULL_HANDLE;
f64 RenderStats::mspf_cpu = 0;
f64 RenderStats::mspf_gpu = 0;
u64 RenderStats::draw_calls = 0;
u64 RenderStats::triangles = 0;
f64 RenderStats::cpu_frame_time_begin = 0;

#ifndef VULKAN_RENDERER_DIST
void RenderStats::Create() {
    VkQueryPoolCreateInfo query_pool_info = { VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };
    query_pool_info.queryType = VK_QUERY_TYPE_TIMESTAMP;
    query_pool_info.queryCount = 2;

    VK_CHECK(vkCreateQueryPool(VulkanDevice::handle, &query_pool_info, 0, &query_pool));
}

void RenderStats::Destroy() {
    vkDestroyQueryPool(VulkanDevice::handle, query_pool, 0);
}

void RenderStats::Begin(VkCommandBuffer cmd_buf) {
    draw_calls = 0;
    triangles = 0;
    cpu_frame_time_begin = glfwGetTime() * 1000;

    vkCmdResetQueryPool(cmd_buf, query_pool, 0, 2);
    vkCmdWriteTimestamp(cmd_buf, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, query_pool, 0);
}

void RenderStats::EndGPU(VkCommandBuffer cmd_buf) {
    vkCmdWriteTimestamp(cmd_buf, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, query_pool, 1);
}

void RenderStats::EndCPU() {
    f64 cpu_frame_time_end = glfwGetTime() * 1000;
    f64 cpu_frame_time_delta = cpu_frame_time_end - cpu_frame_time_begin;
    mspf_cpu = mspf_cpu * 0.95 + cpu_frame_time_delta * 0.05;

    u64 query_results[2];
    
    VK_CHECK(vkGetQueryPoolResults(
        VulkanDevice::handle, query_pool,
        0, 2, sizeof(query_results), query_results,
        sizeof(query_results[0]), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT
    ));

    f64 timestamp_period = VulkanPhysicalDevice::properties.limits.timestampPeriod;
    f64 gpu_frame_time_begin = f64(query_results[0]) * timestamp_period * 1e-6;
    f64 gpu_frame_time_end = f64(query_results[1]) * timestamp_period * 1e-6;
    f64 gpu_frame_time_delta = gpu_frame_time_end - gpu_frame_time_begin;
    mspf_gpu = mspf_gpu * 0.95 + gpu_frame_time_delta * 0.05;
}

void RenderStats::DrawCall() {
    draw_calls++;
}

void RenderStats::CountTriangles(u64 count) {
    triangles += count;
}

void RenderStats::SetTitle(GLFWwindow *window) {
    char title[256];
    sprintf(title, "cpu: %.2fms, gpu: %.2fms, render calls: %lu, triangles: %lu", mspf_cpu, mspf_gpu, draw_calls, triangles);
    glfwSetWindowTitle(window, title);
}
#else
void RenderStats::Create() {}
void RenderStats::Destroy() {}
void RenderStats::Begin(VkCommandBuffer cmd_buf);
void RenderStats::EndGPU(VkCommandBuffer cmd_buf);
void RenderStats::EndCPU(VkCommandBuffer cmd_buf);
void RenderStats::DrawCall() {}
void RenderStats::CountTriangles(u64 count) {}
void RenderStats::SetTitle(GLFWwindow *window) {}
#endif
