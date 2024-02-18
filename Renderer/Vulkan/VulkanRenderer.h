#ifndef VULKAN_RENDERER_H
#define VULKAN_RENDERER_H

#include <assert.h>
#include <stdio.h>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include "Common.h"

#define VK_CHECK(call) \
    if (call != VK_SUCCESS) { \
        LogFatal("Failed Vulkan Call %s:%d\n", __FILE__, __LINE__); \
    }

#include "VulkanInstance.h"
#include "VulkanDevice.h"
#include "VulkanBuffer.h"
#include "VulkanSwapchain.h"
#include "VulkanCommandBuffer.h"
#include "VulkanRenderPass.h"
#include "VulkanFramebuffer.h"
#include "VulkanPipeline.h"

u32 FindMemoryType(u32 type_bits, VkMemoryPropertyFlags flags);

VkSemaphore CreateSemaphore(VkSemaphoreCreateFlags flags=0);
void DestroySemaphore(VkSemaphore handle);

VkFence CreateFence(VkFenceCreateFlags flags=0); 
void DestroyFence(VkFence handle);

struct RenderStats {
    static VkQueryPool query_pool;
	static f64 mspf_cpu;
    static f64 mspf_gpu;
    static u64 draw_calls;
    static u64 triangles;

    static f64 cpu_frame_time_begin;

    static void Create();
    static void Destroy();

    static void Begin(VkCommandBuffer cmd_buf);
    static void EndGPU(VkCommandBuffer cmd_buf);
    static void EndCPU();

    static void DrawCall();
    static void CountTriangles(u64 count);

    static void SetTitle(GLFWwindow *window);
};

#endif
