#include "VulkanRenderer.h"

static bool CheckLayerSupport(const char *name) {
	u32 property_count = 0;
	VK_CHECK(vkEnumerateInstanceLayerProperties(&property_count, 0));

	array<VkLayerProperties> properties(property_count);
	VK_CHECK(vkEnumerateInstanceLayerProperties(&property_count, properties.data()));

	for (u32 i = 0; i < property_count; ++i) {
		if (strcmp(name, properties[i].layerName) == 0) {
			return true;
        }
    }

	return false;
}

VulkanContext VulkanContext::Get(bool enable_layers) {
    VulkanContext ctx;

    if (enable_layers) {
        ctx.layers = {
            "VK_LAYER_KHRONOS_validation"
        };
    } else {
        ctx.layers = {};
    }

    for (const char *layer : ctx.layers) {
        if (!CheckLayerSupport(layer)) {
            LogInfo("Layer %s not supported - Validation layers disabled", layer);
            ctx.layers = {};
            break;
        }
    }

    const char **extensions;
    u32 extension_count;
    extensions = glfwGetRequiredInstanceExtensions(&extension_count);

    ctx.global_extensions.resize(extension_count);
    for (int i = 0; i < extension_count; ++i) {
        ctx.global_extensions[i] = extensions[i];
    }

    ctx.device_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
        VK_KHR_16BIT_STORAGE_EXTENSION_NAME,
        VK_KHR_8BIT_STORAGE_EXTENSION_NAME
    };

    return ctx;
}

VkInstance VulkanInstance::handle = VK_NULL_HANDLE;
VkSurfaceKHR VulkanInstance::surface = VK_NULL_HANDLE;

void VulkanInstance::Create(VulkanContext *ctx, GLFWwindow *window, const char *name) {
    u32 api_version;
    VK_CHECK(vkEnumerateInstanceVersion(&api_version));

    VkApplicationInfo app_info = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
    app_info.pApplicationName = name;
    app_info.applicationVersion = api_version;
    app_info.pEngineName = name;
    app_info.engineVersion = api_version;
    app_info.apiVersion = api_version;

    VkInstanceCreateInfo instance_info = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    instance_info.pApplicationInfo = &app_info;
    instance_info.ppEnabledExtensionNames = ctx->global_extensions.data();
    instance_info.enabledExtensionCount = (u32) ctx->global_extensions.size();
    instance_info.ppEnabledLayerNames = ctx->layers.data();
    instance_info.enabledLayerCount = (u32) ctx->layers.size();

    // VK_CHECK(vkCreateInstance(&instance_info, 0, &handle));
    int result = vkCreateInstance(&instance_info, 0, &handle);
    if (result != VK_SUCCESS) {
        LogFatal("Failed to create Vulkan instance %d", result);
    }

    LogDev("Vulkan version %d.%d", VK_API_VERSION_MAJOR(api_version), VK_API_VERSION_MINOR(api_version));

    VK_CHECK(glfwCreateWindowSurface(VulkanInstance::handle, window, 0, &surface));
}

void VulkanInstance::Destroy() {
    vkDestroySurfaceKHR(handle, surface, 0);
    vkDestroyInstance(handle, 0);
}
