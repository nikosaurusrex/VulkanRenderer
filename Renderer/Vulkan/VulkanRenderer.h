#ifndef VULKAN_RENDERER_H
#define VULKAN_RENDERER_H

#include <assert.h>
#include <stdio.h>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include "Common.h"

#define VK_CHECK(call) \
    if (call != VK_SUCCESS) { \
        LogFatal("Failed Vulkan Call %s:%d\n", __FILE__, __LINE__); \
    }

struct VulkanContext {
    array<const char *> layers;
    array<const char *> global_extensions;
    array<const char *> device_extensions;
    static VulkanContext Get(bool enable_layers);
};

struct VulkanInstance {
    static VkInstance handle;
    static VkSurfaceKHR surface;

    static void Create(VulkanContext *ctx, GLFWwindow *window, const char *name);
    static void Destroy();
};

struct VulkanPhysicalDevice {
    static VkPhysicalDevice handle;
    static VkPhysicalDeviceMemoryProperties memory_properties;
    static VkPhysicalDeviceProperties properties;
    static u32 graphics;
    static u32 present;
    static VkSampleCountFlagBits msaa_samples;

    static VulkanPhysicalDevice *Get();
    static void Pick(VulkanContext *ctx);
};

struct VulkanDevice {
    static VkDevice handle;
    static VkQueue graphics_queue;
    static VkQueue present_queue;
    static u32 graphics_index;
    static u32 present_index;

    static VulkanDevice *Get();
    static void Create(VulkanContext *ctx);
    static void Destroy();
};

struct VulkanCommandPool {
    VkCommandPool handle;

    void Create(u32 queue_family_index);
    void Destroy();
    
    void Reset();
};

struct VulkanCommandBuffers {
    VulkanCommandPool *pool;
    array<VkCommandBuffer> buffers;

    void Create(VulkanCommandPool *pool, u32 count);
    void Destroy();

    void Begin(u32 index, VkCommandBufferUsageFlags flags=0);
    void End(u32 index);

    void Reset(u32 index);
};

struct VulkanImage {
    VkImage handle;
    VkImageView view;
    VkDeviceMemory memory;

    void Create(VkFormat format, u32 width, u32 height, u32 mip_levels, VkSampleCountFlagBits samples, VkImageUsageFlags usage);
    void Destroy();
};

struct VulkanSwapchain {
    VkSwapchainKHR handle;
    VkFormat format;
    VkExtent2D extent;
    array<VkImage> color_images;
    array<VkImageView> color_views;
    VulkanImage depth_image;
    bool vsync;
    
    VkSurfaceFormatKHR ChooseFormat();
    VkPresentModeKHR ChooseSwapPresentMode(bool vsync); 

    void Create(bool vsync);
    void Destroy();

    void CheckResize();
};

struct RenderPass {
    VulkanSwapchain *swapchain;
    VulkanCommandPool graphics_command_pool;
    VulkanCommandBuffers graphics_command_buffers;

    array<VkSemaphore> image_available_semaphores;
    array<VkSemaphore> render_finished_semaphores;
    array<VkFence> in_flight_fences;

    u32 frames_in_flight = 0;
    u32 current_image = 0;
    // need this because current_image is overwritten by vkAcquireNextImageKHR
    u32 current_frame = 0;

    VkCommandBuffer BeginFrame();
    void EndFrame();

    void Create(VulkanSwapchain *swapchain);
    void Destroy();

    void Begin();
    void End();
};

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

struct StorageBuffer {
    VkBuffer buffer;
    VkDeviceMemory memory;
    void *mapped;
    VkDeviceSize size;

    void Create(void *data, VkDeviceSize size);
    void Create(VkDeviceSize size);
    void Destroy();

    void SetData(void *data, VkDeviceSize size);
};

struct IndexBuffer {
    VkBuffer buffer;
    VkDeviceMemory memory;
    u32 count;

    void Create(u32 *data, u32 count, VkCommandPool command_pool);
    void Destroy();
};

struct RenderStats {
    static VkQueryPool query_pool;
	static f64 mspf_cpu;
    static f64 mspf_gpu;
    static u64 draw_calls;
    static u64 triangles;

    static f64 cpu_frame_time_begin;

    static void Create();
    static void Destroy();

    static void Begin(VkCommandBuffer cmd_buf);
    static void EndGPU(VkCommandBuffer cmd_buf);
    static void EndCPU();

    static void DrawCall();
    static void CountTriangles(u64 count);

    static void SetTitle(GLFWwindow *window);
};

// Meh
extern PFN_vkCmdPushDescriptorSetKHR vkCmdPushDescriptorSetFunc;

#endif
