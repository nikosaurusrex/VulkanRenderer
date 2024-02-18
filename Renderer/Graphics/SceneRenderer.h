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
    VkCommandBuffer cmd_buf;
    VkDescriptorUpdateTemplate descriptor_update_template;

    StorageBuffer scene_data_buffer;

    SceneRenderer(VulkanSwapchain *swapchain, RenderPass *render_pass);
    ~SceneRenderer();

    void Begin();
    void End();

    void SetSceneData(SceneData *scene_data);
    void RenderModel(Model *model);
};

#endif
