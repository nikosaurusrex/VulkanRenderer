#ifndef VULKAN_ALLOCATOR_H
#define VULKAN_ALLOCATOR_H

void CreateVulkanAllocator();
void DestroyVulkanAllocator();

VmaAllocation AllocateVulkanBuffer(VkBufferCreateInfo buffer_create_info, VmaMemoryUsage usage, VkBuffer *buffer, void **mapped);
void FreeVulkanBuffer(VkBuffer buffer, VmaAllocation allocation);
void FreeVulkanBufferNoUnmap(VkBuffer buffer, VmaAllocation allocation);

#endif