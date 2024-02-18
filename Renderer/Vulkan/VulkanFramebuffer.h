#ifndef VULKAN_FRAMEBUFFER_H
#define VULKAN_FRAMEBUFFER_H

struct Framebuffer {
    VkFramebuffer handle;          
    Image color_image;
    Image depth_image;

    void Create();
    void Destroy();
};

#endif
