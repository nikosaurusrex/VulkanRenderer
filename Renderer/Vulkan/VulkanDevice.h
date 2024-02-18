#ifndef VULKAN_DEVICE_H
#define VULKAN_DEVICE_H

struct VulkanPhysicalDevice {
    static VkPhysicalDevice handle;
    static VkPhysicalDeviceMemoryProperties memory_properties;
    static VkPhysicalDeviceProperties properties;
    static u32 graphics;
    static u32 present;
    static VkSampleCountFlagBits msaa_samples;

    static VulkanPhysicalDevice *Get();
    static void Pick(VulkanContext *ctx);
};

struct VulkanDevice {
    static VkDevice handle;
    static VkQueue graphics_queue;
    static VkQueue present_queue;
    static u32 graphics_index;
    static u32 present_index;

    static VulkanDevice *Get();
    static void Create(VulkanContext *ctx);
    static void Destroy();
};

// Meh
extern PFN_vkCmdPushDescriptorSetKHR vkCmdPushDescriptorSetFunc;

#endif