#ifndef SCENE_RENDERER_H
#define SCENE_RENDERER_H

#include "Vulkan/VulkanRenderer.h"
#include "Graphics/Model.h"

struct SceneData {
    alignas(16) glm::mat4 projection;
    alignas(16) glm::mat4 view;
    alignas(16) DirectionalLight dir_light;
    alignas(16) u8 num_point_lights;
    alignas(16) PointLight point_lights[10];
};

struct SceneRenderer {
    RenderPass *render_pass;
    Pipeline pipeline;
    VkCommandBuffer cmd_buf;

    StorageBuffer scene_data_buffer;

    SceneRenderer(VulkanSwapchain *swapchain, RenderPass *render_pass);
    ~SceneRenderer();

    void Begin();
    void End();

    void SetSceneData(SceneData *scene_data);
    void RenderModel(Model *model);
};

#endif