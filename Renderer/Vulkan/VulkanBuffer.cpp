#include "VulkanRenderer.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

static VmaAllocation CreateVulkanBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage mem_usage, VkBuffer *buffer, void **mapped) {
    VkDevice device = VulkanDevice::handle;
    
    VkBufferCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size = size;
    info.usage = usage;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    return AllocateVulkanBuffer(info, mem_usage, buffer, mapped);
}

static VkCommandBuffer BeginSingleTimeCommands(VkCommandPool pool) {
    VkDevice device = VulkanDevice::handle;

    VkCommandBufferAllocateInfo info = {};
    info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    info.commandPool        = pool;
    info.commandBufferCount = 1;

    VkCommandBuffer command_buffer;
    vkAllocateCommandBuffers(device, &info, &command_buffer);
    
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(command_buffer, &begin_info);

    return command_buffer;
}

static void EndSingleTimeCommands(VkCommandBuffer command_buffer, VkCommandPool pool) {
    VkDevice device = VulkanDevice::handle;
    VkQueue graphics_queue = VulkanDevice::graphics_queue;

    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo submit = {};
    submit.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount   = 1;
    submit.pCommandBuffers      = &command_buffer;

    vkQueueSubmit(graphics_queue, 1, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphics_queue);

    vkFreeCommandBuffers(device, pool, 1, &command_buffer);
}

static void CopyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size, VkCommandPool command_pool) {
    VkDevice device = VulkanDevice::handle;

    VkCommandBuffer command_buffer = BeginSingleTimeCommands(command_pool);

    VkBufferCopy region;
    region.srcOffset = 0;
    region.dstOffset = 0;
    region.size      = size;
    vkCmdCopyBuffer(command_buffer, src, dst, 1, &region);

    EndSingleTimeCommands(command_buffer, command_pool);
}

static void CopyBufferToImage(VkBuffer src, VkImage image, u32 width, u32 height, VkCommandPool pool) {
    VkCommandBuffer command_buffer = BeginSingleTimeCommands(pool);

    VkBufferImageCopy region = {};
    region.bufferOffset      = 0;
    region.bufferRowLength   = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel       = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount     = 1;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(command_buffer, src, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    EndSingleTimeCommands(command_buffer, pool);
}

static void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout, VkCommandPool pool) {
    VkCommandBuffer command_buffer = BeginSingleTimeCommands(pool);

    VkPipelineStageFlags src_stage;
    VkPipelineStageFlags dst_stage;
    VkAccessFlags src_access;
    VkAccessFlags dst_access;

    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        src_access = 0;
        dst_access = VK_ACCESS_TRANSFER_WRITE_BIT;
        src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else {
        src_access = VK_ACCESS_TRANSFER_WRITE_BIT;
        dst_access = VK_ACCESS_SHADER_READ_BIT;
        src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }

    VkImageMemoryBarrier barrier = CreateBarrier(image, src_access, dst_access, old_layout, new_layout, VK_IMAGE_ASPECT_COLOR_BIT);

    vkCmdPipelineBarrier(command_buffer, src_stage, dst_stage, 0, 0, 0, 0, 0, 1, &barrier);

    EndSingleTimeCommands(command_buffer, pool);
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

    allocation = AllocateVulkanImage(image_info, VMA_MEMORY_USAGE_GPU_ONLY, &handle);

    VkImageAspectFlags aspect_mask = (format == VK_FORMAT_D32_SFLOAT) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

    VkImageViewCreateInfo view_info = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    view_info.image = handle;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = format;
    view_info.subresourceRange.aspectMask = aspect_mask;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = mip_levels;
    view_info.subresourceRange.layerCount = 1;

    VkDevice device = VulkanDevice::handle;
    VK_CHECK(vkCreateImageView(device, &view_info, 0, &view));
}

void Image::Create(const char *filename, VkCommandPool command_pool) {
    int width, height, channels;
    u8 *pixels = stbi_load(filename, &width, &height, &channels, STBI_rgb_alpha);
    VkDeviceSize image_size = width * height * channels;

    if (!pixels) {
        LogFatal("Failed to load image file '%s'", filename);
    }

    VkBuffer staging_buffer;
    void *mapped;
    VmaAllocation staging_allocation = CreateVulkanBuffer(image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, &staging_buffer, &mapped);

    memcpy(mapped, pixels, image_size);

    VkImageCreateInfo image_info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = width;
    image_info.extent.height = height;
    image_info.extent.depth = 1;
    // TODO: Support mipmaps
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.format = VK_FORMAT_R8G8B8A8_SRGB;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    // TODO: Support multisampling
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;

    allocation = AllocateVulkanImage(image_info, VMA_MEMORY_USAGE_GPU_ONLY, &handle);

    CopyBufferToImage(staging_buffer, handle, width, height, command_pool);
    TransitionImageLayout(handle, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, command_pool);

    stbi_image_free(pixels);
    FreeVulkanBuffer(staging_buffer, staging_allocation);

    VkImageViewCreateInfo view_info = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    view_info.image = handle;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = VK_FORMAT_R8G8B8A8_SRGB;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.layerCount = 1;

    VkDevice device = VulkanDevice::handle;
    VK_CHECK(vkCreateImageView(device, &view_info, 0, &view));

    VkSamplerCreateInfo sampler_info = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.anisotropyEnable = VK_TRUE;
    sampler_info.maxAnisotropy = VulkanPhysicalDevice::properties.limits.maxSamplerAnisotropy;
    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    VK_CHECK(vkCreateSampler(device, &sampler_info, 0, &sampler));
}

void Image::Destroy() {
    VkDevice device = VulkanDevice::handle;

    vkDestroyImageView(device, view, 0);

    if (sampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, sampler, 0);
    }

    FreeVulkanImage(handle, allocation);
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
