#ifndef VULKAN_RENDER_PASS
#define VULKAN_RENDER_PASS

struct RenderPass {
    VulkanSwapchain *swapchain;
    VulkanCommandPool graphics_command_pool;
    VulkanCommandBuffers graphics_command_buffers;

    array<VkSemaphore> image_available_semaphores;
    array<VkSemaphore> render_finished_semaphores;
    array<VkFence> in_flight_fences;

    u32 frames_in_flight = 0;
    u32 current_image = 0;
    // need this because current_image is overwritten by vkAcquireNextImageKHR
    u32 current_frame = 0;

    VkCommandBuffer BeginFrame();
    void EndFrame();

    void Create(VulkanSwapchain *swapchain);
    void Destroy();

    void Begin();
    void End();
};

#endif