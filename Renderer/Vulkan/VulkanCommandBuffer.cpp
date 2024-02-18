#include "VulkanRenderer.h"

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
