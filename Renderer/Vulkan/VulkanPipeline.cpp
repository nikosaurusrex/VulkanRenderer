#include "VulkanRenderer.h"

static u32 ReadShaderFile(const char *file_name, u8 **out_buffer) {
    FILE *f = fopen(file_name, "rb");
    if (!f) {
        LogFatal("Failed to open shader file %s", file_name);
        return 0;
    }

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    u8 *buffer = (u8 *)malloc(file_size);
    if (!buffer) {
        LogFatal("Failed to allocate memory for shader file");
        return 0;
    }

    fread(buffer, 1, file_size, f);
    fclose(f);

    *out_buffer = buffer;
    return file_size;
}

void Shader::Create(const char *path) {
    VkShaderModuleCreateInfo shader_info = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    shader_info.codeSize = ReadShaderFile(path, (u8 **) &shader_info.pCode);

    VK_CHECK(vkCreateShaderModule(VulkanDevice::handle, &shader_info, 0, &module));
}

void Shader::Destroy() {
    vkDestroyShaderModule(VulkanDevice::handle, module, 0);
}

void PipelineInfo::AddShader(VkShaderStageFlagBits stage, Shader *shader) {
    shaders[stage] = shader;
}

void PipelineInfo::AddBinding(VkShaderStageFlags stage, VkDescriptorType type) {
    VkDescriptorSetLayoutBinding set_binding = {};
    set_binding.binding = u32(set_bindings.size());
    set_binding.descriptorType = type;
    set_binding.descriptorCount = 1;
    set_binding.stageFlags = stage;

    set_bindings.push_back(set_binding);
}

void PipelineInfo::AddPushConstant(VkShaderStageFlags stage, u32 size) {
    VkPushConstantRange push_constant_range = {};

    push_constant_range.stageFlags = stage;
    push_constant_range.offset = 0;
    push_constant_range.size = size;

    push_constants.push_back(push_constant_range);
}

void Pipeline::Create(VulkanSwapchain *swapchain, PipelineInfo *info) {
    VkDevice device = VulkanDevice::handle;

    VkDescriptorSetLayoutCreateInfo set_create_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    set_create_info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
    set_create_info.bindingCount = u32(info->set_bindings.size());
    set_create_info.pBindings = info->set_bindings.data();

    VK_CHECK(vkCreateDescriptorSetLayout(device, &set_create_info, 0, &descriptor_set_layout));

    VkPipelineLayoutCreateInfo layout_info = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    layout_info.setLayoutCount = 1;
    layout_info.pSetLayouts = &descriptor_set_layout;
    layout_info.pushConstantRangeCount = info->push_constants.size();
    layout_info.pPushConstantRanges = info->push_constants.data();

    VK_CHECK(vkCreatePipelineLayout(device, &layout_info, 0, &layout));

    VkPipelineRenderingCreateInfo rendering_info = { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
    rendering_info.colorAttachmentCount = 1;
    rendering_info.pColorAttachmentFormats = &swapchain->format;
    rendering_info.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;

    array<VkDynamicState> dynamic_states = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamic_state_info = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
    dynamic_state_info.dynamicStateCount = u32(dynamic_states.size());
    dynamic_state_info.pDynamicStates = dynamic_states.data();

    VkPipelineVertexInputStateCreateInfo vertex_input_info = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

    VkPipelineInputAssemblyStateCreateInfo input_assembly_info = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_info.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewport_info = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    viewport_info.viewportCount = 1;
    viewport_info.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterization_info = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    rasterization_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization_info.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterization_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterization_info.lineWidth = 1.0f;

    // TODO: MSAA
    VkPipelineMultisampleStateCreateInfo multisample_info = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    multisample_info.rasterizationSamples = VulkanPhysicalDevice::msaa_samples;
    multisample_info.minSampleShading = 1.0f;
    multisample_info.sampleShadingEnable = VK_TRUE;

    VkPipelineDepthStencilStateCreateInfo depth_stencil_info = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
    depth_stencil_info.depthTestEnable = VK_TRUE;
    depth_stencil_info.depthWriteEnable = VK_TRUE;
    depth_stencil_info.depthCompareOp = VK_COMPARE_OP_LESS;

    VkPipelineColorBlendAttachmentState color_blend_attachment = {};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_TRUE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo color_blend_info = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    color_blend_info.pAttachments = &color_blend_attachment;
    color_blend_info.attachmentCount = 1;

    array<VkPipelineShaderStageCreateInfo> shaders;

    for (auto &&[stage, shader] : info->shaders) {
        VkPipelineShaderStageCreateInfo stage_info = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        stage_info.stage = stage;
        stage_info.module = shader->module;
        stage_info.pName = "main";

        shaders.push_back(stage_info);
    }

    VkGraphicsPipelineCreateInfo pipeline_info = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
    pipeline_info.pNext = &rendering_info;
    pipeline_info.stageCount = u32(shaders.size());
    pipeline_info.pStages = shaders.data();
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly_info;
    pipeline_info.pViewportState = &viewport_info;
    pipeline_info.pRasterizationState = &rasterization_info;
    pipeline_info.pMultisampleState = &multisample_info;
    pipeline_info.pDepthStencilState = &depth_stencil_info;
    pipeline_info.pColorBlendState = &color_blend_info;
    pipeline_info.pDynamicState = &dynamic_state_info;
    pipeline_info.layout = layout;

    VK_CHECK(vkCreateGraphicsPipelines(device, cache, 1, &pipeline_info, 0, &handle));
}

void Pipeline::Destroy() {
    VkDevice device = VulkanDevice::handle;

    vkDestroyPipeline(device, handle, 0);
    vkDestroyPipelineLayout(device, layout, 0);
    vkDestroyDescriptorSetLayout(device, descriptor_set_layout, 0);
}

VkDescriptorUpdateTemplate CreateDescriptorUpdateTemplate(Pipeline *pipeline, PipelineInfo *info, VkPipelineBindPoint bind_point) {
    // Note: this assumes we we use every binding from 0 to n
    array<VkDescriptorUpdateTemplateEntry> entries(info->set_bindings.size());
     
    for (u32 i = 0; i < info->set_bindings.size(); ++i) {
        entries[i].dstBinding = i;
        entries[i].dstArrayElement = 0;
        entries[i].descriptorCount = 1;
        entries[i].descriptorType = info->set_bindings[i].descriptorType;
        entries[i].offset = i * sizeof(DescriptorInfo);
        entries[i].stride = sizeof(DescriptorInfo);
    }

    VkDescriptorUpdateTemplateCreateInfo update_template_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_CREATE_INFO };

    update_template_info.descriptorUpdateEntryCount = u32(entries.size());
    update_template_info.pDescriptorUpdateEntries = entries.data();
    update_template_info.templateType = VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_PUSH_DESCRIPTORS_KHR;
    update_template_info.descriptorSetLayout = pipeline->descriptor_set_layout;
    update_template_info.pipelineBindPoint = bind_point;
    update_template_info.pipelineLayout = pipeline->layout;

    VkDescriptorUpdateTemplate update_template;
    VK_CHECK(vkCreateDescriptorUpdateTemplate(VulkanDevice::handle, &update_template_info, 0, &update_template));

    return update_template;
}
