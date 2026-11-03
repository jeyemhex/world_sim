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

typedef struct {
    GLFWwindow* window;
    VkInstance instance;
    VkSurfaceKHR surface;
} VulkanContext;

static VulkanContext ctx = {0};

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
        return -1;
    }

    // Specify the application info
    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = title,
        .applicationVersion = VK_MAKE_VERSION(1,0,0),
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

    printf("Vulkan 1.3 instance and window surface initialised successfully.\n");
    return 0;
}

int vk_should_close(void) {
    glfwPollEvents();
    return glfwWindowShouldClose(ctx.window);
}

void vk_cleanup(void) {
    vkDestroySurfaceKHR(ctx.instance, ctx.surface, NULL);
    vkDestroyInstance(ctx.instance, NULL);
    glfwDestroyWindow(ctx.window);
    glfwTerminate();
    printf("Vulkan resources cleaned up.\n");
}
