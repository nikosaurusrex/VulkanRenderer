#ifndef VULKAN_INSTANCE_H
#define VULKAN_INSTANCE_H

struct VulkanContext {
    array<const char *> layers;
    array<const char *> global_extensions;
    array<const char *> device_extensions;
    static VulkanContext Get(bool enable_layers);
};

struct VulkanInstance {
    static VkInstance handle;
    static VkSurfaceKHR surface;

    static void Create(VulkanContext *ctx, GLFWwindow *window, const char *name);
    static void Destroy();
};

#endif