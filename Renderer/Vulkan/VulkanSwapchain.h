#ifndef VULKAN_SWAPCHAIN_H
#define VULKAN_SWAPCHAIN_H

struct VulkanSwapchain {
    VkSwapchainKHR handle;
    VkFormat format;
    VkExtent2D extent;
    array<VkImage> images;
    array<VkImageView> views;
    bool vsync;
    
    VkSurfaceFormatKHR ChooseFormat();
    VkPresentModeKHR ChooseSwapPresentMode(bool vsync); 

    void Create(bool vsync);
    void Destroy();

    void CheckResize(RenderImages *images);
};

#endif
