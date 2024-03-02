#ifndef MASTER_RENDERER_H
#define MASTER_RENDERER_H

#include "Vulkan/VulkanRenderer.h"

struct MasterRenderer {
    RenderPass *render_pass;
    VkCommandBuffer cmd_buf;

    RenderImages render_images;

    MasterRenderer(RenderPass *render_pass);
    ~MasterRenderer();

    VkCommandBuffer Begin();
    void End();
};

#endif