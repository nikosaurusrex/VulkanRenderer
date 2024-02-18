#include "VulkanRenderer.h"


VkSurfaceFormatKHR VulkanSwapchain::ChooseFormat() {
    VkPhysicalDevice device = VulkanPhysicalDevice::handle;

    u32 format_count;
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(device, VulkanInstance::surface, &format_count, 0));

    array<VkSurfaceFormatKHR> formats(format_count);
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(device, VulkanInstance::surface, &format_count, formats.data()));

    for (u32 i = 0; i < formats.size(); ++i) {
        if (formats[i].format == VK_FORMAT_R8G8B8_UNORM || formats[i].format == VK_FORMAT_B8G8R8_UNORM) {
            return formats[i];
        }
    }

    return formats[0];
}

VkPresentModeKHR VulkanSwapchain::ChooseSwapPresentMode(bool vsync) {
    VkPhysicalDevice device = VulkanPhysicalDevice::handle;

    u32 present_mode_count;
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(device, VulkanInstance::surface, &present_mode_count, 0));

    array<VkPresentModeKHR> present_modes(present_mode_count);
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(device, VulkanInstance::surface, &present_mode_count, present_modes.data()));

    VkPresentModeKHR mode = VK_PRESENT_MODE_FIFO_KHR;
    if (vsync) {
        return mode;
    }

    for (u32 i = 0; i < present_modes.size(); ++i) {
        if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            mode = present_modes[i];
        } else if (present_modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR && mode != VK_PRESENT_MODE_MAILBOX_KHR) {
            mode = present_modes[i];
        }
    }

    return mode;
}

void VulkanSwapchain::Create(bool vsync) {
    this->vsync = vsync;

    VkSurfaceCapabilitiesKHR capabilities;
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VulkanPhysicalDevice::handle, VulkanInstance::surface, &capabilities));
    
    VkSurfaceFormatKHR surface_format = ChooseFormat();

    format = surface_format.format;
    extent = capabilities.currentExtent;

    VkSwapchainCreateInfoKHR swap_chain_info = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    swap_chain_info.surface = VulkanInstance::surface;
    swap_chain_info.minImageCount = capabilities.minImageCount;
    swap_chain_info.imageFormat = format;
    swap_chain_info.imageColorSpace = surface_format.colorSpace;
    swap_chain_info.imageExtent = extent;
    swap_chain_info.imageArrayLayers = 1;

    // VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT if we want to render to the swapchain images
    // VK_IMAGE_USAGE_TRANSFER_DST_BIT if we want to copy the swapchain images to another image (in the case we use a framebuffer)
    swap_chain_info.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    swap_chain_info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swap_chain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swap_chain_info.presentMode = ChooseSwapPresentMode(vsync);

    if (VulkanDevice::graphics_index == VulkanDevice::present_index) {
        swap_chain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    } else {
        swap_chain_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swap_chain_info.queueFamilyIndexCount = 2;

        u32 queue_family_indices[] = { VulkanDevice::graphics_index, VulkanDevice::present_index };
        swap_chain_info.pQueueFamilyIndices = queue_family_indices;
    }

    VK_CHECK(vkCreateSwapchainKHR(VulkanDevice::handle, &swap_chain_info, 0, &handle));

    u32 image_count;
    VK_CHECK(vkGetSwapchainImagesKHR(VulkanDevice::handle, handle, &image_count, 0));

    images.resize(image_count);
    views.resize(image_count);
    VK_CHECK(vkGetSwapchainImagesKHR(VulkanDevice::handle, handle, &image_count, images.data()));

    for (u32 i = 0; i < image_count; ++i) {
        VkImageViewCreateInfo view_info = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        view_info.image = images[i];
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = format;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = 1;

        VK_CHECK(vkCreateImageView(VulkanDevice::handle, &view_info, 0, &views[i]));
    }
}

void VulkanSwapchain::Destroy() {
    VkDevice device = VulkanDevice::handle;

    for (VkImageView view : views) {
        vkDestroyImageView(device, view, 0);
    }

    vkDestroySwapchainKHR(VulkanDevice::handle, handle, 0);
}

void VulkanSwapchain::CheckResize(RenderImages *images) {
    VkSurfaceCapabilitiesKHR capabilities;
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VulkanPhysicalDevice::handle, VulkanInstance::surface, &capabilities));

    if (!images->color_image.handle) {
        images->Create(this);
    }

    if (capabilities.currentExtent.width != extent.width || capabilities.currentExtent.height != extent.height) {
        Destroy();
        Create(vsync);
        images->Destroy();
        images->Create(this);

        extent = capabilities.currentExtent;

        VK_CHECK(vkDeviceWaitIdle(VulkanDevice::handle));
    }
}
