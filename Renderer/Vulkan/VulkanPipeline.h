#ifndef VULKAN_PIPELINE_H
#define VULKAN_PIPELINE_H

struct Shader {
    VkShaderModule module;

    void Create(const char *path);
    void Destroy();
};

struct Vertex {
    glm::vec<3, f32> position;
    glm::vec<4, u8> normal;
    glm::vec<2, f32> tex_coord;
};

struct PipelineInfo {
    map<VkShaderStageFlagBits, Shader *> shaders;
    array<VkDescriptorSetLayoutBinding> set_bindings;
    array<VkPushConstantRange> push_constants;
    
    void AddShader(VkShaderStageFlagBits stage, Shader *shader);
    void AddBinding(VkShaderStageFlags stage, VkDescriptorType type);
    void AddPushConstant(VkShaderStageFlags stage, u32 size);
};

struct Pipeline {
    VkPipeline handle;
    VkPipelineLayout layout;
    VkDescriptorSetLayout descriptor_set_layout;
    // TODO: cache
    VkPipelineCache cache = 0;

    void Create(VulkanSwapchain *swapchain, PipelineInfo *info);
    void Destroy();
};

union DescriptorInfo {
    VkDescriptorBufferInfo buffer;
    VkDescriptorImageInfo image;

    DescriptorInfo(Image *img) {
        image.sampler = img->sampler;
        image.imageView = img->view;
        image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    DescriptorInfo(StorageBuffer *storage_buffer) {
        buffer.buffer = storage_buffer->buffer;
        buffer.offset = 0;
        buffer.range = VK_WHOLE_SIZE;
    }
};

VkDescriptorUpdateTemplate CreateDescriptorUpdateTemplate(Pipeline *pipeline, PipelineInfo *info, VkPipelineBindPoint bind_point);

#endif
