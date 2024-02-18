#include "VulkanRenderer.h"

void RenderPass::Create(VulkanSwapchain *swapchain) {
    this->swapchain = swapchain;

    frames_in_flight = u32(swapchain->images.size());

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
    for (u32 i = 0; i < swapchain->images.size(); ++i) {
        DestroySemaphore(image_available_semaphores[i]);
        DestroySemaphore(render_finished_semaphores[i]);
        DestroyFence(in_flight_fences[i]);
    }

    graphics_command_buffers.Destroy();
    graphics_command_pool.Destroy();
}

VkCommandBuffer RenderPass::BeginFrame(RenderImages *images) {
    swapchain->CheckResize(images);

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

void RenderPass::Begin(RenderImages *images) {
    VkCommandBuffer graphics_command_buffer = graphics_command_buffers.buffers[current_frame];

    VkRenderingAttachmentInfo color_attachment = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
    color_attachment.imageView = images->color_image.view;
    color_attachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.clearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };

    VkRenderingAttachmentInfo depth_attachment = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
    depth_attachment.imageView = images->depth_image.view;
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

    VkImageMemoryBarrier barriers[2] = {
        CreateBarrier(
            images->color_image.handle, 0, 0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT
        ),
        CreateBarrier(
            images->depth_image.handle, 0, 0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT
        )
    };

    vkCmdPipelineBarrier(
        graphics_command_buffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_DEPENDENCY_BY_REGION_BIT,
        0, 0, 0, 0, ARRAY_SIZE(barriers), barriers
    );

    vkCmdBeginRendering(graphics_command_buffer, &rendering_info);

    VkViewport viewport = { 0.0f, 0.0f, f32(swapchain->extent.width), f32(swapchain->extent.height), 0.0f, 1.0f };
    VkRect2D scissor = { { 0, 0 }, swapchain->extent };

    vkCmdSetViewport(graphics_command_buffer, 0, 1, &viewport);
    vkCmdSetScissor(graphics_command_buffer, 0, 1, &scissor);
}

void RenderPass::End(RenderImages *images) {
    VkCommandBuffer graphics_command_buffer = graphics_command_buffers.buffers[current_frame];

    vkCmdEndRendering(graphics_command_buffer);

    VkImageMemoryBarrier barriers[2] = {
        CreateBarrier(
            images->color_image.handle,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT
        ),
        CreateBarrier(
            swapchain->images[current_image],
            0, VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT
        )
    };

    vkCmdPipelineBarrier(
        graphics_command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT,
        0, 0, 0, 0, ARRAY_SIZE(barriers), barriers
    );

    VkImageCopy copy_region = {};
    copy_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy_region.srcSubresource.layerCount = 1;
    copy_region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy_region.dstSubresource.layerCount = 1;
    copy_region.extent = { swapchain->extent.width, swapchain->extent.height, 1 };

    vkCmdCopyImage(
        graphics_command_buffer,
        images->color_image.handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        swapchain->images[current_image], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &copy_region
    );

    VkImageMemoryBarrier copy_barrier = CreateBarrier(
        swapchain->images[current_image],
        VK_ACCESS_TRANSFER_WRITE_BIT, 0,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_ASPECT_COLOR_BIT
    );

    vkCmdPipelineBarrier(
        graphics_command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_DEPENDENCY_BY_REGION_BIT,
        0, 0, 0, 0, 1, &copy_barrier
    );
}
