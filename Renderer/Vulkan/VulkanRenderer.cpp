#include "VulkanRenderer.h"

PFN_vkCmdPushDescriptorSetKHR vkCmdPushDescriptorSetFunc = 0;

VulkanContext VulkanContext::Get(bool enable_layers) {
    VulkanContext ctx;

    if (enable_layers) {
        ctx.layers = {
       //     "VK_LAYER_LUNARG_core_validation"
        };
    } else {
        ctx.layers = {};
    }

    const char **extensions;
    u32 extension_count;
    extensions = glfwGetRequiredInstanceExtensions(&extension_count);

    ctx.global_extensions.resize(extension_count);
    for (int i = 0; i < extension_count; ++i) {
        ctx.global_extensions[i] = extensions[i];
    }

    ctx.device_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
        VK_KHR_16BIT_STORAGE_EXTENSION_NAME,
        VK_KHR_8BIT_STORAGE_EXTENSION_NAME
    };

    return ctx;
}

VkInstance VulkanInstance::handle = VK_NULL_HANDLE;
VkSurfaceKHR VulkanInstance::surface = VK_NULL_HANDLE;

void VulkanInstance::Create(VulkanContext *ctx, GLFWwindow *window, const char *name) {
    u32 api_version;
    VK_CHECK(vkEnumerateInstanceVersion(&api_version));

    VkApplicationInfo app_info = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
    app_info.pApplicationName = name;
    app_info.applicationVersion = api_version;
    app_info.pEngineName = name;
    app_info.engineVersion = api_version;
    app_info.apiVersion = api_version;

    VkInstanceCreateInfo instance_info = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    instance_info.pApplicationInfo = &app_info;
    instance_info.ppEnabledExtensionNames = ctx->global_extensions.data();
    instance_info.enabledExtensionCount = (u32) ctx->global_extensions.size();
    instance_info.ppEnabledLayerNames = ctx->layers.data();
    instance_info.enabledLayerCount = (u32) ctx->layers.size();

    // VK_CHECK(vkCreateInstance(&instance_info, 0, &handle));
    int result = vkCreateInstance(&instance_info, 0, &handle);
    if (result != VK_SUCCESS) {
        LogFatal("Failed to create Vulkan instance %d", result);
    }

    LogDev("Vulkan version %d.%d", VK_API_VERSION_MAJOR(api_version), VK_API_VERSION_MINOR(api_version));

    VK_CHECK(glfwCreateWindowSurface(VulkanInstance::handle, window, 0, &surface));
}

void VulkanInstance::Destroy() {
    vkDestroySurfaceKHR(handle, surface, 0);
    vkDestroyInstance(handle, 0);
}

VkPhysicalDevice VulkanPhysicalDevice::handle = 0;
VkPhysicalDeviceMemoryProperties VulkanPhysicalDevice::memory_properties = {};
VkPhysicalDeviceProperties VulkanPhysicalDevice::properties = {};
u32 VulkanPhysicalDevice::graphics = 0;
u32 VulkanPhysicalDevice::present = 0;
VkSampleCountFlagBits VulkanPhysicalDevice::msaa_samples = VK_SAMPLE_COUNT_1_BIT;

void VulkanPhysicalDevice::Pick(VulkanContext *ctx) {
    u32 device_count;
    VK_CHECK(vkEnumeratePhysicalDevices(VulkanInstance::handle, &device_count, 0));

    array<VkPhysicalDevice> devices(device_count);
    VK_CHECK(vkEnumeratePhysicalDevices(VulkanInstance::handle, &device_count, devices.data()));

    for (VkPhysicalDevice dev : devices) {
        VkPhysicalDeviceProperties properties;

        vkGetPhysicalDeviceProperties(dev, &properties);

        if (properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            continue;
        }

        u32 extension_count;
        vkEnumerateDeviceExtensionProperties(dev, 0, &extension_count, 0); 

        array<VkExtensionProperties> avaiable_extensions(extension_count);

        vkEnumerateDeviceExtensionProperties(dev, 0, &extension_count, avaiable_extensions.data());
        
        set<string> required_extensions(ctx->device_extensions.begin(), ctx->device_extensions.end());
        for (auto &extension : avaiable_extensions) {
            required_extensions.erase(extension.extensionName);
        }

        if (!required_extensions.empty()) {
            continue;
        }

        u32 family_count;
        vkGetPhysicalDeviceQueueFamilyProperties(dev, &family_count, 0);

        array<VkQueueFamilyProperties> families(family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(dev, &family_count, families.data());

        u32 graphics_index = -1;
        u32 present_index = -1;
        for (u32 i = 0; i < families.size(); ++i) {
            VkQueueFamilyProperties family = families[i];

            if (family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                graphics_index = i;
            }

            VkBool32 present_support = false;

            vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, VulkanInstance::surface, &present_support);
            if (present_support) {
                present_index = i;
            }
        }

        if (graphics_index == -1 || present_index == -1) {
            continue;
        }

        // Set selected device
        LogDev("Found GPU %s", properties.deviceName);
        handle = dev;
        graphics = graphics_index;
        present = present_index;
        break;
    }

    vkGetPhysicalDeviceMemoryProperties(handle, &memory_properties);
    vkGetPhysicalDeviceProperties(handle, &properties);

    VkSampleCountFlags msaa_flags = properties.limits.framebufferColorSampleCounts & properties.limits.framebufferDepthSampleCounts;

    VkSampleCountFlagBits samples;
    if (msaa_flags & VK_SAMPLE_COUNT_16_BIT) samples = VK_SAMPLE_COUNT_16_BIT;
    else if (msaa_flags & VK_SAMPLE_COUNT_8_BIT) samples = VK_SAMPLE_COUNT_8_BIT;
    else if (msaa_flags & VK_SAMPLE_COUNT_4_BIT) samples = VK_SAMPLE_COUNT_4_BIT;
    else if (msaa_flags & VK_SAMPLE_COUNT_2_BIT) samples = VK_SAMPLE_COUNT_2_BIT;
    else samples = VK_SAMPLE_COUNT_1_BIT;

    // TODO: enable msaa
    // _instance->msaa_samples = samples;
    msaa_samples = VK_SAMPLE_COUNT_1_BIT;

    if (handle == VK_NULL_HANDLE) {
        LogFatal("Failed to find suitable GPU");
    }
}

VkDevice VulkanDevice::handle = VK_NULL_HANDLE;
VkQueue VulkanDevice::graphics_queue = VK_NULL_HANDLE;
VkQueue VulkanDevice::present_queue = VK_NULL_HANDLE;
u32 VulkanDevice::graphics_index = ~0u;
u32 VulkanDevice::present_index = ~0u;

void VulkanDevice::Create(VulkanContext *ctx) {
    VkPhysicalDeviceFeatures features_core = {};
    features_core.sampleRateShading = VK_TRUE;

    VkPhysicalDeviceVulkan13Features features13 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
	features13.dynamicRendering = VK_TRUE;
    features13.synchronization2 = VK_TRUE;

    VkPhysicalDeviceVulkan12Features features12 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
    features12.shaderInt8 = VK_TRUE;
    features12.uniformAndStorageBuffer8BitAccess = VK_TRUE;

    const u32 queue_create_info_count = 2;
    VkDeviceQueueCreateInfo queue_create_infos[queue_create_info_count];

    f32 queue_priority = 1.0f;

    VkDeviceQueueCreateInfo graphics_queue_info = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
    graphics_queue_info.queueFamilyIndex = VulkanPhysicalDevice::graphics;
    graphics_queue_info.queueCount = 1;
    graphics_queue_info.pQueuePriorities = &queue_priority;
    queue_create_infos[0] = graphics_queue_info;
    
    VkDeviceQueueCreateInfo present_queue_info = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
    present_queue_info.queueFamilyIndex = VulkanPhysicalDevice::present;
    present_queue_info.queueCount = 1;
    present_queue_info.pQueuePriorities = &queue_priority;
    queue_create_infos[1] = present_queue_info;

    VkDeviceCreateInfo device_info = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    device_info.ppEnabledExtensionNames = ctx->device_extensions.data();
    device_info.enabledExtensionCount = (u32) ctx->device_extensions.size();
    device_info.ppEnabledLayerNames = ctx->layers.data();
    device_info.enabledLayerCount = (u32) ctx->layers.size();
    device_info.pEnabledFeatures = &features_core;
    device_info.queueCreateInfoCount = queue_create_info_count;
    device_info.pQueueCreateInfos = queue_create_infos;
    device_info.pNext = &features13;

    features13.pNext = &features12;

    VkDevice device;
    VK_CHECK(vkCreateDevice(VulkanPhysicalDevice::handle, &device_info, 0, &device));

    vkGetDeviceQueue(device, VulkanPhysicalDevice::graphics, 0, &graphics_queue);

    vkGetDeviceQueue(device, VulkanPhysicalDevice::present, 0, &present_queue);

    handle = device;
    graphics_index = VulkanPhysicalDevice::graphics;
    present_index = VulkanPhysicalDevice::present;

    vkCmdPushDescriptorSetFunc = (PFN_vkCmdPushDescriptorSetKHR) vkGetDeviceProcAddr(device, "vkCmdPushDescriptorSetKHR");
}

void VulkanDevice::Destroy() {
    vkDestroyDevice(handle, 0);
}

void VulkanCommandPool::Create(u32 queue_family_index) {
    VkCommandPoolCreateInfo command_pool_info = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    command_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_info.queueFamilyIndex = queue_family_index;

    VK_CHECK(vkCreateCommandPool(VulkanDevice::handle, &command_pool_info, 0, &handle));
}

void VulkanCommandPool::Destroy() {
    vkDestroyCommandPool(VulkanDevice::handle, handle, 0);
}

void VulkanCommandPool::Reset() {
    VK_CHECK(vkResetCommandPool(VulkanDevice::handle, handle, 0));
}


void VulkanCommandBuffers::Create(VulkanCommandPool *pool, u32 count) {
    this->pool = pool;

    VkCommandBufferAllocateInfo command_buffer_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    command_buffer_info.commandPool = pool->handle;
    command_buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_info.commandBufferCount = count;

    buffers.resize(count);

    VK_CHECK(vkAllocateCommandBuffers(VulkanDevice::handle, &command_buffer_info, buffers.data()));
}

void VulkanCommandBuffers::Destroy() {
    vkFreeCommandBuffers(VulkanDevice::handle, pool->handle, (u32) buffers.size(), buffers.data());
}

void VulkanCommandBuffers::Begin(u32 index, VkCommandBufferUsageFlags flags) {
    VkCommandBufferBeginInfo begin_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    begin_info.flags = flags;

    VK_CHECK(vkBeginCommandBuffer(buffers[index], &begin_info));
}

void VulkanCommandBuffers::End(u32 index) {
    VK_CHECK(vkEndCommandBuffer(buffers[index]));
}

void VulkanCommandBuffers::Reset(u32 index) {
    vkResetCommandBuffer(buffers[index], 0);
}

static u32 FindMemoryType(u32 type_bits, VkMemoryPropertyFlags flags) {
    for (u32 i = 0; i < VulkanPhysicalDevice::memory_properties.memoryTypeCount; ++i) {
        if ((type_bits & (1 << i)) && (VulkanPhysicalDevice::memory_properties.memoryTypes[i].propertyFlags & flags) == flags) {
            return i;
        }
    }

    LogFatal("Failed to find suitable memory type");
    return 0;
}

void VulkanImage::Create(VkFormat format, u32 width, u32 height, u32 mip_levels, VkSampleCountFlagBits samples, VkImageUsageFlags usage) {
    VkImageCreateInfo image_info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = format;
    image_info.extent.width = width;
    image_info.extent.height = height;
    image_info.extent.depth = 1;
    image_info.mipLevels = mip_levels;
    image_info.arrayLayers = 1;
    image_info.samples = samples;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = usage;

    VkDevice device = VulkanDevice::handle;
    VK_CHECK(vkCreateImage(device, &image_info, 0, &handle));

    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(device, handle, &memory_requirements);


    VkMemoryAllocateInfo allocate_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocate_info.allocationSize = memory_requirements.size;
    allocate_info.memoryTypeIndex = FindMemoryType(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VK_CHECK(vkAllocateMemory(device, &allocate_info, 0, &memory));
    VK_CHECK(vkBindImageMemory(device, handle, memory, 0));

    VkImageAspectFlags aspect_mask = (format == VK_FORMAT_D32_SFLOAT) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

    VkImageViewCreateInfo view_info = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    view_info.image = handle;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = format;
    view_info.subresourceRange.aspectMask = aspect_mask;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = mip_levels;
    view_info.subresourceRange.layerCount = 1;

    VK_CHECK(vkCreateImageView(device, &view_info, 0, &view));
}

void VulkanImage::Destroy() {
    VkDevice device = VulkanDevice::handle;

    vkDestroyImageView(device, view, 0);
    vkDestroyImage(device, handle, 0);
    vkFreeMemory(device, memory, 0);
}

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
    swap_chain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
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

    color_images.resize(image_count);
    color_views.resize(image_count);
    VK_CHECK(vkGetSwapchainImagesKHR(VulkanDevice::handle, handle, &image_count, color_images.data()));

    for (u32 i = 0; i < image_count; ++i) {
        VkImageViewCreateInfo view_info = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        view_info.image = color_images[i];
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = format;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = 1;

        VK_CHECK(vkCreateImageView(VulkanDevice::handle, &view_info, 0, &color_views[i]));
    }

    depth_image.Create(
        VK_FORMAT_D32_SFLOAT, extent.width, extent.height, 1,
        VulkanPhysicalDevice::msaa_samples, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

void VulkanSwapchain::Destroy() {
    VkDevice device = VulkanDevice::handle;

    for (VkImageView view : color_views) {
        vkDestroyImageView(device, view, 0);
    }

    depth_image.Destroy();

    vkDestroySwapchainKHR(VulkanDevice::handle, handle, 0);
}

void VulkanSwapchain::CheckResize() {
    VkSurfaceCapabilitiesKHR capabilities;
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VulkanPhysicalDevice::handle, VulkanInstance::surface, &capabilities));

    if (capabilities.currentExtent.width != extent.width || capabilities.currentExtent.height != extent.height) {
        Destroy();
        Create(vsync);

        extent = capabilities.currentExtent;

        VK_CHECK(vkDeviceWaitIdle(VulkanDevice::handle));
    }
}

VkSemaphore CreateSemaphore(VkSemaphoreCreateFlags flags=0) {
    VkSemaphoreCreateInfo semaphore_info = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    semaphore_info.flags = flags;

    VkSemaphore handle;
    VK_CHECK(vkCreateSemaphore(VulkanDevice::handle, &semaphore_info, 0, &handle));
    return handle;
}

void DestroySemaphore(VkSemaphore handle) {
    vkDestroySemaphore(VulkanDevice::handle, handle, 0);
}

VkFence CreateFence(VkFenceCreateFlags flags=0) {
    VkFenceCreateInfo fence_info = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fence_info.flags = flags;

    VkFence handle;
    VK_CHECK(vkCreateFence(VulkanDevice::handle, &fence_info, 0, &handle));
    return handle;
}

void DestroyFence(VkFence handle) {
    vkDestroyFence(VulkanDevice::handle, handle, 0);
}

void RenderPass::Create(VulkanSwapchain *swapchain) {
    this->swapchain = swapchain;

    frames_in_flight = (u32) swapchain->color_images.size();

    graphics_command_pool.Create(VulkanDevice::graphics_index);
    graphics_command_buffers.Create(&graphics_command_pool, frames_in_flight);

    image_available_semaphores.resize(frames_in_flight);
    render_finished_semaphores.resize(frames_in_flight);
    in_flight_fences.resize(frames_in_flight);

    for (u32 i = 0; i < frames_in_flight; ++i) {
        image_available_semaphores[i] = CreateSemaphore();
        render_finished_semaphores[i] = CreateSemaphore();
        in_flight_fences[i] = CreateFence(VK_FENCE_CREATE_SIGNALED_BIT);
    }
}

void RenderPass::Destroy() {
    for (u32 i = 0; i < swapchain->color_images.size(); ++i) {
        DestroySemaphore(image_available_semaphores[i]);
        DestroySemaphore(render_finished_semaphores[i]);
        DestroyFence(in_flight_fences[i]);
    }

    graphics_command_buffers.Destroy();
    graphics_command_pool.Destroy();
}

VkCommandBuffer RenderPass::BeginFrame() {
    swapchain->CheckResize();

    VK_CHECK(vkWaitForFences(VulkanDevice::handle, 1, &in_flight_fences[current_frame], VK_TRUE, UINT64_MAX));

    VkResult result = vkAcquireNextImageKHR(
        VulkanDevice::handle, swapchain->handle,
        UINT64_MAX, image_available_semaphores[current_frame],
        VK_NULL_HANDLE, &current_image
    );
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        LogFatal("Failed to acquire swap chain image");
    }
    
    VK_CHECK(vkResetFences(VulkanDevice::handle, 1, &in_flight_fences[current_frame]));

    graphics_command_buffers.Reset(current_frame);
    graphics_command_buffers.Begin(current_frame, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    return graphics_command_buffers.buffers[current_frame];
}

void RenderPass::EndFrame() {
    graphics_command_buffers.End(current_frame);

    VkPipelineStageFlags submit_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submit_info = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &image_available_semaphores[current_frame];
    submit_info.pWaitDstStageMask = &submit_stage_mask;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &graphics_command_buffers.buffers[current_frame];
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &render_finished_semaphores[current_frame];

    VK_CHECK(vkQueueSubmit(VulkanDevice::graphics_queue, 1, &submit_info, in_flight_fences[current_frame]));

    VkPresentInfoKHR present_info = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &render_finished_semaphores[current_frame];
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain->handle;
    present_info.pImageIndices = &current_image;

    VkResult result = vkQueuePresentKHR(VulkanDevice::present_queue, &present_info);
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        LogFatal("Failed to present swap chain image");
    }

    current_frame = (current_frame + 1) % frames_in_flight;
}

void RenderPass::Begin() {
    VkCommandBuffer graphics_command_buffer = graphics_command_buffers.buffers[current_frame];

    VkRenderingAttachmentInfo color_attachment = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
    color_attachment.imageView = swapchain->color_views.at(current_image);
    color_attachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.clearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };

    VkRenderingAttachmentInfo depth_attachment = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
    depth_attachment.imageView = swapchain->depth_image.view;
    depth_attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.clearValue.depthStencil = { 1.0f, 0 };

    VkRenderingInfo rendering_info = { VK_STRUCTURE_TYPE_RENDERING_INFO };
    rendering_info.renderArea.extent = swapchain->extent;
    rendering_info.layerCount = 1;
    rendering_info.colorAttachmentCount = 1;
    rendering_info.pColorAttachments = &color_attachment;
    rendering_info.pDepthAttachment = &depth_attachment;

    VkImageMemoryBarrier2 color_image_barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
    color_image_barrier.srcStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    color_image_barrier.srcAccessMask = VK_ACCESS_NONE;
    color_image_barrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    color_image_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    color_image_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_image_barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_image_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    color_image_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    color_image_barrier.image = swapchain->color_images.at(current_image);
    color_image_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    color_image_barrier.subresourceRange.baseMipLevel = 0;
    color_image_barrier.subresourceRange.levelCount = 1;
    color_image_barrier.subresourceRange.baseArrayLayer = 0;
    color_image_barrier.subresourceRange.layerCount = 1;
    
    VkDependencyInfo dependency_info = {};
    dependency_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependency_info.pNext = NULL;
    dependency_info.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    dependency_info.memoryBarrierCount = 0;
    dependency_info.pMemoryBarriers = NULL;
    dependency_info.bufferMemoryBarrierCount = 0;
    dependency_info.pBufferMemoryBarriers = NULL;
    dependency_info.imageMemoryBarrierCount = 1;
    dependency_info.pImageMemoryBarriers = &color_image_barrier;
    
    vkCmdPipelineBarrier2(graphics_command_buffer, &dependency_info);

    vkCmdBeginRendering(graphics_command_buffer, &rendering_info);

    VkViewport viewport = { 0.0f, 0.0f, (f32) swapchain->extent.width, (f32) swapchain->extent.height, 0.0f, 1.0f };
    VkRect2D scissor = { { 0, 0 }, swapchain->extent };

    vkCmdSetViewport(graphics_command_buffer, 0, 1, &viewport);
    vkCmdSetScissor(graphics_command_buffer, 0, 1, &scissor);
}

void RenderPass::End() {
    VkCommandBuffer graphics_command_buffer = graphics_command_buffers.buffers[current_frame];

    vkCmdEndRendering(graphics_command_buffer);

    VkImageMemoryBarrier2 image_barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
    image_barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    image_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    image_barrier.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
    image_barrier.dstAccessMask = VK_ACCESS_NONE;
    image_barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    image_barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    image_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_barrier.image = swapchain->color_images.at(current_image);
    image_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_barrier.subresourceRange.baseMipLevel = 0;
    image_barrier.subresourceRange.levelCount = 1;
    image_barrier.subresourceRange.baseArrayLayer = 0;
    image_barrier.subresourceRange.layerCount = 1;

    VkDependencyInfo dependency_info = { VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
    dependency_info.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    dependency_info.imageMemoryBarrierCount = 1;
    dependency_info.pImageMemoryBarriers = &image_barrier;
    
    vkCmdPipelineBarrier2(graphics_command_buffer, &dependency_info);
}

static u32 ReadShaderFile(const char *file_name, u8 **out_buffer) {
    FILE *f = fopen(file_name, "rb");
    if (!f) {
        LogFatal("Failed to open shader file %s", file_name);
        return 0;
    }

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    u8 *buffer = (u8 *)malloc(file_size);
    if (!buffer) {
        LogFatal("Failed to allocate memory for shader file");
        return 0;
    }

    fread(buffer, 1, file_size, f);
    fclose(f);

    *out_buffer = buffer;
    return file_size;
}

void Shader::Create(const char *path) {
    VkShaderModuleCreateInfo shader_info = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    shader_info.codeSize = ReadShaderFile(path, (u8 **) &shader_info.pCode);

    VK_CHECK(vkCreateShaderModule(VulkanDevice::handle, &shader_info, 0, &module));
}

void Shader::Destroy() {
    vkDestroyShaderModule(VulkanDevice::handle, module, 0);
}

void PipelineInfo::AddShader(VkShaderStageFlagBits stage, Shader *shader) {
    shaders[stage] = shader;
}

void PipelineInfo::AddBinding(VkShaderStageFlags stage, VkDescriptorType type) {
    VkDescriptorSetLayoutBinding set_binding = {};
    set_binding.binding = (u32) set_bindings.size();
    set_binding.descriptorType = type;
    set_binding.descriptorCount = 1;
    set_binding.stageFlags = stage;

    set_bindings.push_back(set_binding);
}

void PipelineInfo::AddPushConstant(VkShaderStageFlags stage, u32 size) {
    VkPushConstantRange push_constant_range = {};

    push_constant_range.stageFlags = stage;
    push_constant_range.offset = 0;
    push_constant_range.size = size;

    push_constants.push_back(push_constant_range);
}

void Pipeline::Create(VulkanSwapchain *swapchain, PipelineInfo *info) {
    VkDevice device = VulkanDevice::handle;

    VkDescriptorSetLayoutCreateInfo set_create_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    set_create_info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
    set_create_info.bindingCount = (u32) info->set_bindings.size();
    set_create_info.pBindings = info->set_bindings.data();

    VK_CHECK(vkCreateDescriptorSetLayout(device, &set_create_info, 0, &descriptor_set_layout));

    VkPipelineLayoutCreateInfo layout_info = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    layout_info.setLayoutCount = 1;
    layout_info.pSetLayouts = &descriptor_set_layout;
    layout_info.pushConstantRangeCount = info->push_constants.size();
    layout_info.pPushConstantRanges = info->push_constants.data();

    VK_CHECK(vkCreatePipelineLayout(device, &layout_info, 0, &layout));

    VkPipelineRenderingCreateInfo rendering_info = { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
    rendering_info.colorAttachmentCount = 1;
    rendering_info.pColorAttachmentFormats = &swapchain->format;
    rendering_info.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;

    array<VkDynamicState> dynamic_states = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamic_state_info = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
    dynamic_state_info.dynamicStateCount = (u32) dynamic_states.size();
    dynamic_state_info.pDynamicStates = dynamic_states.data();

    VkPipelineVertexInputStateCreateInfo vertex_input_info = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

    VkPipelineInputAssemblyStateCreateInfo input_assembly_info = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_info.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewport_info = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    viewport_info.viewportCount = 1;
    viewport_info.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterization_info = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    rasterization_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization_info.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterization_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterization_info.lineWidth = 1.0f;

    // TODO: MSAA
    VkPipelineMultisampleStateCreateInfo multisample_info = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    multisample_info.rasterizationSamples = VulkanPhysicalDevice::msaa_samples;
    multisample_info.minSampleShading = 1.0f;
    multisample_info.sampleShadingEnable = VK_TRUE;

    VkPipelineDepthStencilStateCreateInfo depth_stencil_info = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
    depth_stencil_info.depthTestEnable = VK_TRUE;
    depth_stencil_info.depthWriteEnable = VK_TRUE;
    depth_stencil_info.depthCompareOp = VK_COMPARE_OP_LESS;

    VkPipelineColorBlendAttachmentState color_blend_attachment = {};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_TRUE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo color_blend_info = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    color_blend_info.pAttachments = &color_blend_attachment;
    color_blend_info.attachmentCount = 1;

    array<VkPipelineShaderStageCreateInfo> shaders;

    for (auto &&[stage, shader] : info->shaders) {
        VkPipelineShaderStageCreateInfo stage_info = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        stage_info.stage = stage;
        stage_info.module = shader->module;
        stage_info.pName = "main";

        shaders.push_back(stage_info);
    }

    VkGraphicsPipelineCreateInfo pipeline_info = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
    pipeline_info.pNext = &rendering_info;
    pipeline_info.stageCount = (u32) shaders.size();
    pipeline_info.pStages = shaders.data();
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly_info;
    pipeline_info.pViewportState = &viewport_info;
    pipeline_info.pRasterizationState = &rasterization_info;
    pipeline_info.pMultisampleState = &multisample_info;
    pipeline_info.pDepthStencilState = &depth_stencil_info;
    pipeline_info.pColorBlendState = &color_blend_info;
    pipeline_info.pDynamicState = &dynamic_state_info;
    pipeline_info.layout = layout;

    VK_CHECK(vkCreateGraphicsPipelines(device, cache, 1, &pipeline_info, 0, &handle));
}

void Pipeline::Destroy() {
    VkDevice device = VulkanDevice::handle;

    vkDestroyPipeline(device, handle, 0);
    vkDestroyPipelineLayout(device, layout, 0);
    vkDestroyDescriptorSetLayout(device, descriptor_set_layout, 0);
}

static void CreateVulkanBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer *buffer, VkDeviceMemory *memory) {
    VkDevice device = VulkanDevice::handle;
    
    VkBufferCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size = size;
    info.usage = usage;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &info, 0, buffer) != VK_SUCCESS) {
        LogFatal("Failed to create vertex buffer");
    }

    VkMemoryRequirements mem_reqs;
    vkGetBufferMemoryRequirements(device, *buffer, &mem_reqs);

    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_reqs.size;
    alloc_info.memoryTypeIndex = FindMemoryType(mem_reqs.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &alloc_info, 0, memory) != VK_SUCCESS) {
        LogFatal("Failed to allocate vertex buffer memory");
    }

    vkBindBufferMemory(device, *buffer, *memory, 0);
}

static void CopyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size, VkCommandPool command_pool) {
    VkDevice device = VulkanDevice::handle;
    VkQueue graphics_queue = VulkanDevice::graphics_queue;
    
    VkCommandBufferAllocateInfo info = {};
    info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    info.commandPool        = command_pool;
    info.commandBufferCount = 1;

    VkCommandBuffer command_buffer;
    vkAllocateCommandBuffers(device, &info, &command_buffer);
    
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(command_buffer, &begin_info);

    VkBufferCopy region;
    region.srcOffset = 0;
    region.dstOffset = 0;
    region.size      = size;
    vkCmdCopyBuffer(command_buffer, src, dst, 1, &region);

    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo submit = {};
    submit.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount   = 1;
    submit.pCommandBuffers      = &command_buffer;

    vkQueueSubmit(graphics_queue, 1, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphics_queue);

    vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
}

void StorageBuffer::Create(void *data, VkDeviceSize size) {
    Create(size);
    SetData(data, size);
}

void StorageBuffer::Create(VkDeviceSize size) {
    this->size = size;

    VkDevice device = VulkanDevice::handle;

    CreateVulkanBuffer(size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &buffer, &memory);

    vkMapMemory(device, memory, 0, size, 0, &mapped);
}

void StorageBuffer::Destroy() {
    VkDevice device = VulkanDevice::handle;

    vkUnmapMemory(device, memory);
    vkDestroyBuffer(device, buffer, 0);
    vkFreeMemory(device, memory, 0);
}

void StorageBuffer::SetData(void *data, VkDeviceSize size) {
    memcpy(mapped, data, size);
}

void IndexBuffer::Create(u32 *data, u32 count, VkCommandPool command_pool) {
    this->count = count;

    VkDevice device = VulkanDevice::handle;

    VkDeviceSize size = (u64)count * sizeof(u32);

    VkBuffer staging_buffer;
    VkDeviceMemory staging_memory;

    CreateVulkanBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &staging_buffer, &staging_memory);

    void *mapped;
    vkMapMemory(device, staging_memory, 0, size, 0, &mapped);
    memcpy(mapped, data, size);
    vkUnmapMemory(device, staging_memory);

    CreateVulkanBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &buffer, &memory);

    CopyBuffer(staging_buffer, buffer, size, command_pool);

    vkDestroyBuffer(device, staging_buffer, 0);
    vkFreeMemory(device, staging_memory, 0);
}

void IndexBuffer::Destroy() {
    VkDevice device = VulkanDevice::handle;
    
    vkDestroyBuffer(device, buffer, 0);
    vkFreeMemory(device, memory, 0);
}

VkQueryPool RenderStats::query_pool = VK_NULL_HANDLE;
f64 RenderStats::mspf_cpu = 0;
f64 RenderStats::mspf_gpu = 0;
u64 RenderStats::draw_calls = 0;
u64 RenderStats::triangles = 0;
f64 RenderStats::cpu_frame_time_begin = 0;

#ifndef VULKAN_RENDERER_DIST
void RenderStats::Create() {
    VkQueryPoolCreateInfo query_pool_info = { VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };
    query_pool_info.queryType = VK_QUERY_TYPE_TIMESTAMP;
    query_pool_info.queryCount = 2;

    VK_CHECK(vkCreateQueryPool(VulkanDevice::handle, &query_pool_info, 0, &query_pool));
}

void RenderStats::Destroy() {
    vkDestroyQueryPool(VulkanDevice::handle, query_pool, 0);
}

void RenderStats::Begin(VkCommandBuffer cmd_buf) {
    draw_calls = 0;
    triangles = 0;
    cpu_frame_time_begin = glfwGetTime() * 1000;

    vkCmdResetQueryPool(cmd_buf, query_pool, 0, 2);
    vkCmdWriteTimestamp(cmd_buf, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, query_pool, 0);
}

void RenderStats::EndGPU(VkCommandBuffer cmd_buf) {
    vkCmdWriteTimestamp(cmd_buf, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, query_pool, 1);
}

void RenderStats::EndCPU() {
    f64 cpu_frame_time_end = glfwGetTime() * 1000;
    f64 cpu_frame_time_delta = cpu_frame_time_end - cpu_frame_time_begin;
    mspf_cpu = mspf_cpu * 0.95 + cpu_frame_time_delta * 0.05;

    u64 query_results[2];
    
    VK_CHECK(vkGetQueryPoolResults(
        VulkanDevice::handle, query_pool,
        0, 2, sizeof(query_results), query_results,
        sizeof(query_results[0]), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT
    ));

    f64 timestamp_period = VulkanPhysicalDevice::properties.limits.timestampPeriod;
    f64 gpu_frame_time_begin = f64(query_results[0]) * timestamp_period * 1e-6;
    f64 gpu_frame_time_end = f64(query_results[1]) * timestamp_period * 1e-6;
    f64 gpu_frame_time_delta = gpu_frame_time_end - gpu_frame_time_begin;
    mspf_gpu = mspf_gpu * 0.95 + gpu_frame_time_delta * 0.05;
}

void RenderStats::DrawCall() {
    draw_calls++;
}

void RenderStats::CountTriangles(u64 count) {
    triangles += count;
}

void RenderStats::SetTitle(GLFWwindow *window) {
    char title[256];
    sprintf(title, "cpu: %.2fms, gpu: %.2fms, render calls: %lu, triangles: %lu", mspf_cpu, mspf_gpu, draw_calls, triangles);
    glfwSetWindowTitle(window, title);
}
#else
void RenderStats::Create() {}
void RenderStats::Destroy() {}
void RenderStats::Begin(VkCommandBuffer cmd_buf);
void RenderStats::EndGPU(VkCommandBuffer cmd_buf);
void RenderStats::EndCPU(VkCommandBuffer cmd_buf);
void RenderStats::DrawCall() {}
void RenderStats::CountTriangles(u64 count) {}
void RenderStats::SetTitle(GLFWwindow *window) {}
#endif
