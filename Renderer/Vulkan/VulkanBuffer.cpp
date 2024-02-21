#include "VulkanRenderer.h"

static VmaAllocation CreateVulkanBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage mem_usage, VkBuffer *buffer, void **mapped) {
    VkDevice device = VulkanDevice::handle;
    
    VkBufferCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size = size;
    info.usage = usage;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    return AllocateVulkanBuffer(info, mem_usage, buffer, mapped);
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

void Image::Create(VkFormat format, u32 width, u32 height, u32 mip_levels, VkSampleCountFlagBits samples, VkImageUsageFlags usage) {
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

void Image::Destroy() {
    VkDevice device = VulkanDevice::handle;

    vkDestroyImageView(device, view, 0);
    vkDestroyImage(device, handle, 0);
    vkFreeMemory(device, memory, 0);
}

VkImageMemoryBarrier CreateBarrier(VkImage image, VkAccessFlags src_access, VkAccessFlags dst_access, VkImageLayout old_layout, VkImageLayout new_layout, VkImageAspectFlags aspect_mask) {
    VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    barrier.srcAccessMask = src_access;
    barrier.dstAccessMask = dst_access;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = aspect_mask;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    return barrier;
}

void StorageBuffer::Create(void *data, VkDeviceSize size, VkCommandPool command_pool) {
    Create(size);
    SetData(data, size, command_pool);
}

void StorageBuffer::Create(VkDeviceSize size) {
    this->size = size;

    allocation = CreateVulkanBuffer(
        size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY,
        &buffer, 0
    );
}

void StorageBuffer::Destroy() {
    FreeVulkanBufferNoUnmap(buffer, allocation);
}

void StorageBuffer::SetData(void *data, VkDeviceSize size, VkCommandPool command_pool) {
    VkBuffer staging_buffer;
    void *mapped;
    VmaAllocation staging_allocation = CreateVulkanBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, &staging_buffer, &mapped);

    memcpy(mapped, data, size);
    
    CopyBuffer(staging_buffer, buffer, size, command_pool);

    FreeVulkanBuffer(staging_buffer, staging_allocation);
}

void IndexBuffer::Create(u32 *data, u32 count, VkCommandPool command_pool) {
    this->count = count;

    VkDeviceSize size = u64(count) * sizeof(u32);

    VkBuffer staging_buffer;
    void *mapped;
    VmaAllocation staging_allocation = CreateVulkanBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, &staging_buffer, &mapped);

    memcpy(mapped, data, size);
    
    CreateVulkanBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY, &buffer, 0);
    CopyBuffer(staging_buffer, buffer, size, command_pool);

    FreeVulkanBuffer(staging_buffer, staging_allocation);
}

void IndexBuffer::Destroy() {
    FreeVulkanBufferNoUnmap(buffer, allocation);
}

void RenderImages::Create(VulkanSwapchain *swapchain) {
    color_image.Create(
        swapchain->format,
        swapchain->extent.width,
        swapchain->extent.height,
        1,
        VK_SAMPLE_COUNT_1_BIT,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT 
    );

    depth_image.Create(
        VK_FORMAT_D32_SFLOAT,
        swapchain->extent.width,
        swapchain->extent.height,
        1,
        VK_SAMPLE_COUNT_1_BIT,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
    );
}

void RenderImages::Destroy() {
    color_image.Destroy();
    depth_image.Destroy();
}
