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

    RenderStats::Create();
}

SceneRenderer::~SceneRenderer() {
    RenderStats::Destroy();

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

    VkDescriptorBufferInfo scene_data_buffer_info;
    scene_data_buffer_info.buffer = scene_data_buffer.buffer;
    scene_data_buffer_info.offset = 0;
    scene_data_buffer_info.range = size;

    VkWriteDescriptorSet scene_write_descriptors[1] = {};
    scene_write_descriptors[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    scene_write_descriptors[0].dstBinding = 0;
    scene_write_descriptors[0].descriptorCount = 1;
    scene_write_descriptors[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    scene_write_descriptors[0].pBufferInfo = &scene_data_buffer_info;

    vkCmdPushDescriptorSetFunc(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0, ARRAY_SIZE(scene_write_descriptors), scene_write_descriptors);
}

void SceneRenderer::RenderModel(Model *model) {
    VkDescriptorBufferInfo material_buffer_info;
    material_buffer_info.buffer = model->materials_buffer->buffer;
    material_buffer_info.offset = 0;
    material_buffer_info.range = model->materials_buffer->size;

    VkWriteDescriptorSet material_write_descriptors[1] = {};
    material_write_descriptors[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    material_write_descriptors[0].dstBinding = 2;
    material_write_descriptors[0].descriptorCount = 1;
    material_write_descriptors[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    material_write_descriptors[0].pBufferInfo = &material_buffer_info;

    vkCmdPushDescriptorSetFunc(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0, ARRAY_SIZE(material_write_descriptors), material_write_descriptors);

    for (Mesh *mesh : model->meshes) {
        VkWriteDescriptorSet mesh_write_descriptors[1] = {};

        VkDescriptorBufferInfo vertex_buffer_info = {};
        VkDeviceSize vertex_buffer_size = mesh->vertices_buffer->size;
        vertex_buffer_info.buffer = mesh->vertices_buffer->buffer;
        vertex_buffer_info.offset = 0;
        vertex_buffer_info.range = vertex_buffer_size;

        RenderStats::CountTriangles(vertex_buffer_size / sizeof(Vertex) / 3);

        mesh_write_descriptors[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        mesh_write_descriptors[0].dstBinding = 1;
        mesh_write_descriptors[0].descriptorCount = 1;
        mesh_write_descriptors[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        mesh_write_descriptors[0].pBufferInfo = &vertex_buffer_info;

        vkCmdPushDescriptorSetFunc(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0, ARRAY_SIZE(mesh_write_descriptors), mesh_write_descriptors);

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
