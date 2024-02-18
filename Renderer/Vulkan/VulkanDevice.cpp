#include "VulkanRenderer.h"

PFN_vkCmdPushDescriptorSetKHR vkCmdPushDescriptorSetFunc = 0;
PFN_vkCmdPushDescriptorSetWithTemplateKHR vkCmdPushDescriptorSetWithTemplateFunc = 0;

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
    device_info.enabledExtensionCount = u32(ctx->device_extensions.size());
    device_info.ppEnabledLayerNames = ctx->layers.data();
    device_info.enabledLayerCount = u32(ctx->layers.size());
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
    vkCmdPushDescriptorSetWithTemplateFunc = (PFN_vkCmdPushDescriptorSetWithTemplateKHR) vkGetDeviceProcAddr(device, "vkCmdPushDescriptorSetWithTemplateKHR");
}

void VulkanDevice::Destroy() {
    vkDestroyDevice(handle, 0);
}
