#ifndef VULKAN_SWAPCHAIN_H
#define VULKAN_SWAPCHAIN_H

struct VulkanSwapchain {
    VkSwapchainKHR handle;
    VkFormat format;
    VkExtent2D extent;
    array<VkImage> color_images;
    array<VkImageView> color_views;
    Image depth_image;
    bool vsync;
    
    VkSurfaceFormatKHR ChooseFormat();
    VkPresentModeKHR ChooseSwapPresentMode(bool vsync); 

    void Create(bool vsync);
    void Destroy();

    void CheckResize();
};

#endif
