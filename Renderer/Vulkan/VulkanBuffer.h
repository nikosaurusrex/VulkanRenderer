#ifndef VULKAN_BUFFER_H
#define VULKAN_BUFFER_H

struct Image {
    VkImage handle = VK_NULL_HANDLE;
    VkImageView view;
    VkDeviceMemory memory;

    void Create(VkFormat format, u32 width, u32 height, u32 mip_levels, VkSampleCountFlagBits samples, VkImageUsageFlags usage);
    void Destroy();
};

VkImageMemoryBarrier CreateBarrier(VkImage image, VkAccessFlags src_access, VkAccessFlags dst_access, VkImageLayout old_layout, VkImageLayout new_layout, VkImageAspectFlags aspect_mask);

struct StorageBuffer {
    VkBuffer buffer;
    VmaAllocation allocation;
    void *mapped = 0;
    VkDeviceSize size;

    void Create(void *data, VkDeviceSize size, VkCommandPool command_pool);
    void Create(VkDeviceSize size);
    void Destroy();

    void SetData(void *data, VkDeviceSize size, VkCommandPool command_pool);
};

struct IndexBuffer {
    VkBuffer buffer;
    VmaAllocation allocation;
    u32 count;

    void Create(u32 *data, u32 count, VkCommandPool command_pool);
    void Destroy();
};

// Should probably move this
struct VulkanSwapchain;
struct RenderImages {
    Image color_image;
    Image depth_image;
    
    void Create(VulkanSwapchain *swapchain);
    void Destroy();
};

#endif
