#ifndef SCENE_RENDERER_H
#define SCENE_RENDERER_H

#include "Vulkan/VulkanRenderer.h"
#include "Graphics/Model.h"

struct alignas(16) SceneData {
    glm::mat4 projection;
    glm::mat4 view;
    DirectionalLight dir_light;
    u8 num_point_lights;
    PointLight point_lights[10];
};

struct SceneRenderer {
    RenderPass *render_pass;
    Pipeline pipeline;
    VkDescriptorUpdateTemplate descriptor_update_template;

    StorageBuffer scene_data_buffer;
    VkCommandBuffer cmd_buf;

    SceneRenderer(VulkanSwapchain *swapchain, RenderPass *render_pass);
    ~SceneRenderer();

    void Begin(VkCommandBuffer cmd_buf);
    void End();

    void SetSceneData(SceneData *scene_data);
    void RenderModel(Model *model);
};

#endif
