#ifndef VULKAN_BUFFER_H
#define VULKAN_BUFFER_H

struct Image {
    VkImage handle;
    VkImageView view;
    VkDeviceMemory memory;

    void Create(VkFormat format, u32 width, u32 height, u32 mip_levels, VkSampleCountFlagBits samples, VkImageUsageFlags usage);
    void Destroy();
};

struct StorageBuffer {
    VkBuffer buffer;
    VkDeviceMemory memory;
    void *mapped;
    VkDeviceSize size;

    void Create(void *data, VkDeviceSize size);
    void Create(VkDeviceSize size);
    void Destroy();

    void SetData(void *data, VkDeviceSize size);
};

struct IndexBuffer {
    VkBuffer buffer;
    VkDeviceMemory memory;
    u32 count;

    void Create(u32 *data, u32 count, VkCommandPool command_pool);
    void Destroy();
};

#endif
