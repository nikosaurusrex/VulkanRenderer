#include "SceneRenderer.h"

SceneRenderer::SceneRenderer(VulkanSwapchain *swapchain, RenderPass *render_pass) : render_pass(render_pass) {
    Shader vertex_shader, fragment_shader;
    vertex_shader.Create("Renderer/Assets/Shaders/simple.vert.spv");
    fragment_shader.Create("Renderer/Assets/Shaders/simple.frag.spv");

    PipelineInfo pipeline_info;
    pipeline_info.AddShader(VK_SHADER_STAGE_VERTEX_BIT, &vertex_shader);
    pipeline_info.AddShader(VK_SHADER_STAGE_FRAGMENT_BIT, &fragment_shader);
	pipeline_info.AddBinding(VK_SHADER_STAGE_VERTEX_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	pipeline_info.AddBinding(VK_SHADER_STAGE_VERTEX_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	pipeline_info.AddBinding(VK_SHADER_STAGE_VERTEX_BIT, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    pipeline_info.AddPushConstant(VK_SHADER_STAGE_VERTEX_BIT, sizeof(MeshData));

    pipeline.Create(swapchain, &pipeline_info);

    scene_data_buffer.Create(sizeof(SceneData));

    // TODO: check if ok
    vertex_shader.Destroy();
    fragment_shader.Destroy();

    descriptor_update_template = CreateDescriptorUpdateTemplate(&pipeline, &pipeline_info, VK_PIPELINE_BIND_POINT_GRAPHICS);

    RenderStats::Create();
}

SceneRenderer::~SceneRenderer() {
    RenderStats::Destroy();

    vkDestroyDescriptorUpdateTemplate(VulkanDevice::handle, descriptor_update_template, 0);

    scene_data_buffer.Destroy();
    pipeline.Destroy();
}

void SceneRenderer::Begin() {
    cmd_buf = render_pass->BeginFrame();

    RenderStats::Begin(cmd_buf);

    render_pass->Begin();
    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.handle);
}

void SceneRenderer::End() {
    render_pass->End();

    RenderStats::EndGPU(cmd_buf);

    render_pass->EndFrame();

    RenderStats::EndCPU();
}

void SceneRenderer::SetSceneData(SceneData *scene_data) {
    u32 size = 3 * sizeof(glm::mat4) + 16 + scene_data->num_point_lights * sizeof(PointLight);
    
    scene_data_buffer.SetData(scene_data, size);
}

void SceneRenderer::RenderModel(Model *model) {
    for (Mesh *mesh : model->meshes) {
        DescriptorInfo updates[3] = { 
            &scene_data_buffer,
            mesh->vertices_buffer,
            model->materials_buffer
        };

        vkCmdPushDescriptorSetWithTemplateFunc(cmd_buf, descriptor_update_template, pipeline.layout, 0, updates);

        RenderStats::CountTriangles(mesh->vertices_buffer->size / sizeof(Vertex) / 3);

        MeshData mesh_data;
        mesh_data.material_index = mesh->material_index;
        // TODO: Change
        mesh_data.model_matrix = model->transformation;

        // TODO: change sometime in future to not use push constants?
        // The issue is that we would need some dynamic uniforms to update uniform buffers
        // We can't use dynamic buffers though because of we use push decriptors
        vkCmdPushConstants(cmd_buf, pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshData), &mesh_data);

        vkCmdBindIndexBuffer(cmd_buf, mesh->index_buffer->buffer, 0, VK_INDEX_TYPE_UINT32);

        RenderStats::DrawCall();
        vkCmdDrawIndexed(cmd_buf, mesh->index_buffer->count, 1, 0, 0, 0);
    }
}
