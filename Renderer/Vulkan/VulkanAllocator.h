#ifndef VULKAN_ALLOCATOR_H
#define VULKAN_ALLOCATOR_H

void CreateVulkanAllocator();
void DestroyVulkanAllocator();

VmaAllocation AllocateVulkanBuffer(VkBufferCreateInfo buffer_create_info, VmaMemoryUsage usage, VkBuffer *buffer, void **mapped);
void FreeVulkanBuffer(VkBuffer buffer, VmaAllocation allocation);
void FreeVulkanBufferNoUnmap(VkBuffer buffer, VmaAllocation allocation);

VmaAllocation AllocateVulkanImage(VkImageCreateInfo image_create_info, VmaMemoryUsage usage, VkImage *image);
void FreeVulkanImage(VkImage image, VmaAllocation allocation);

#endif