#include "VulkanRenderer.h"

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