#ifndef VULKAN_COMMAND_BUFFER_H
#define VULKAN_COMMAND_BUFFER_H

struct VulkanCommandPool {
    VkCommandPool handle;

    void Create(u32 queue_family_index);
    void Destroy();
    
    void Reset();
};

struct VulkanCommandBuffers {
    VulkanCommandPool *pool;
    array<VkCommandBuffer> buffers;

    void Create(VulkanCommandPool *pool, u32 count);
    void Destroy();

    void Begin(u32 index, VkCommandBufferUsageFlags flags=0);
    void End(u32 index);

    void Reset(u32 index);
};

#endif