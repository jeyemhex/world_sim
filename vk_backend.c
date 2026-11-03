/*==============================================================================*
 * VK_BACKEND
 *------------------------------------------------------------------------------*
 * Author:  Ed Higgins <ed.j.higgins@gmail.com>
 *------------------------------------------------------------------------------*
 * Version: 0.1.1, 2026-11-03
 *------------------------------------------------------------------------------*
 * This code is distributed under the GLWTPL
 *==============================================================================*/

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct {
    GLFWwindow* window;
    VkInstance instance;
    VkSurfaceKHR surface;
    VkPhysicalDevice physical_device;
    VkDevice device;
    uint32_t queue_family_index;
    VkQueue queue;
} VulkanContext;

static VulkanContext ctx = {0};

static int pick_physical_device(void) {
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(ctx.instance, &device_count, NULL);
    if (device_count == 0) {
        fprintf(stderr, "Failed to find GPUs with Vulkan support.\n");
        return -1;
    }

    VkPhysicalDevice* devices = malloc(sizeof(VkPhysicalDevice) * device_count);
    vkEnumeratePhysicalDevices(ctx.instance, &device_count, devices);

    // Pick the first discrete GPU (or fall back to the first available GPU)
    ctx.physical_device = devices[0];
    for (uint32_t i=0; i<device_count; i++) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(devices[i], &props);
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            ctx.physical_device = devices[i];
            break;
        }
    }
    free(devices);

    VkPhysicalDeviceProperties chosen_props;
    vkGetPhysicalDeviceProperties(ctx.physical_device, &chosen_props);
    printf("Selected GPU: %s\n", chosen_props.deviceName);

    // Find a queue family that supports graphics, compute and surface presentation
    uint32_t queue_family_count= 0;
    vkGetPhysicalDeviceQueueFamilyProperties(ctx.physical_device, &queue_family_count, NULL);
    VkQueueFamilyProperties* queue_families = malloc(sizeof(VkQueueFamilyProperties) * queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(ctx.physical_device, &queue_family_count, queue_families);

    bool found = false;
    for (uint32_t i=0; i<queue_family_count; i++) {
        VkBool32 present_support = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(ctx.physical_device, i, ctx.surface, &present_support);

        VkQueueFlags flags = queue_families[i].queueFlags;
        if ((flags & VK_QUEUE_GRAPHICS_BIT) && (flags & VK_QUEUE_COMPUTE_BIT) && present_support) {
            ctx.queue_family_index = i;
            found = true;
            break;
        }
    }
    free(queue_families);

    if (!found) {
        fprintf(stderr, "Failed to dfind a unified Graphics+Compute+Present queue family.\n");
        return -1;
    }

    printf("Using queue family index: %u\n", ctx.queue_family_index);
    return 0;
}

static int create_logical_device(void) {
    float queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = ctx.queue_family_index,
        .queueCount = 1,
        .pQueuePriorities = &queue_priority
    };

    // Enable Vulkan 1.3 features: dynamic rendering & synchronization2
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .dynamicRendering = VK_TRUE,
        .synchronization2 = VK_TRUE
    };

    const char* device_extensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    VkDeviceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &features13,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_create_info,
        .enabledExtensionCount = 1,
        .ppEnabledExtensionNames = device_extensions
    };

    if (vkCreateDevice(ctx.physical_device, &create_info, NULL, &ctx.device) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create logical device\n");
        return -1;
    }

    vkGetDeviceQueue(ctx.device, ctx.queue_family_index, 0, &ctx.queue);
    return 0;
}

int vk_init(int width, int height, const char* title) {
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialise GLFW\n");
        return -1;
    }

    // Tell GLFW not to create an OpenGL context
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    ctx.window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (!ctx.window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }

    // Specify the application info
    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = title,
        .applicationVersion = VK_MAKE_VERSION(1,0,0),
        .pEngineName = "FortranWorldSim",
        .engineVersion = VK_MAKE_VERSION(1,0,0),
        .apiVersion = VK_API_VERSION_1_3
    };

    // Get required extensions from GFLW
    uint32_t glfw_ext_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_ext_count);

    // Enable Khronos validation layers for debugging
    const char* validation_layers[] = { "VK_LAYER_KHRONOS_validation" };

    VkInstanceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledExtensionCount = glfw_ext_count,
        .ppEnabledExtensionNames = glfw_extensions,
        .enabledLayerCount = 1,
        .ppEnabledLayerNames = validation_layers
    };

    if (vkCreateInstance(&create_info, NULL, &ctx.instance) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create Vulkan instance\n");
        return -1;
    }

    // Create the window surface
    if (glfwCreateWindowSurface(ctx.instance, ctx.window, NULL, &ctx.surface) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create window surface\n");
        return -1;
    }

    if (pick_physical_device() != 0) return -1;
    if (create_logical_device() != 0) return -1;

    printf("Vulkan 1.3 instance and window surface initialised successfully.\n");
    return 0;
}

int vk_should_close(void) {
    glfwPollEvents();
    return glfwWindowShouldClose(ctx.window);
}

void vk_cleanup(void) {
    vkDestroyDevice(ctx.device, NULL);
    vkDestroySurfaceKHR(ctx.instance, ctx.surface, NULL);
    vkDestroyInstance(ctx.instance, NULL);
    glfwDestroyWindow(ctx.window);
    glfwTerminate();
    printf("Vulkan resources cleaned up.\n");
}
