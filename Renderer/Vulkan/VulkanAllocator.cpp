#define VMA_IMPLEMENTATION
#include "VulkanRenderer.h"

static VmaAllocator allocator;

void CreateVulkanAllocator() {
    VmaAllocatorCreateInfo create_info = {};
    create_info.vulkanApiVersion = VK_API_VERSION_1_3;
    create_info.instance = VulkanInstance::handle;
    create_info.physicalDevice = VulkanPhysicalDevice::handle;
    create_info.device = VulkanDevice::handle;

    vmaCreateAllocator(&create_info, &allocator);
}

void DestroyVulkanAllocator() {
    vmaDestroyAllocator(allocator);
}

VmaAllocation AllocateVulkanBuffer(VkBufferCreateInfo buffer_create_info, VmaMemoryUsage usage, VkBuffer *buffer, void **mapped) {
    VmaAllocationCreateInfo alloc_create_info = {};
    alloc_create_info.usage = usage;

    VmaAllocation allocation;
    vmaCreateBuffer(allocator, &buffer_create_info, &alloc_create_info, buffer, &allocation, 0);

    if (mapped) {
        vmaMapMemory(allocator, allocation, mapped);
    }

    return allocation;
}

void FreeVulkanBuffer(VkBuffer buffer, VmaAllocation allocation) {
    vmaUnmapMemory(allocator, allocation);
    vmaDestroyBuffer(allocator, buffer, allocation);
}

void FreeVulkanBufferNoUnmap(VkBuffer buffer, VmaAllocation allocation) {
    vmaDestroyBuffer(allocator, buffer, allocation);
}