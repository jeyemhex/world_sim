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

    // Swapchain
    VkSwapchainKHR swapchain;
    VkFormat swapchain_format;
    VkExtent2D swapchain_extent;
    uint32_t image_count;
    VkImage* images;
    VkImageView* image_views;

    // Commands & sync
    VkCommandPool cmd_pool;
    VkCommandBuffer cmd_buf;
    VkSemaphore image_available_sem;
    VkSemaphore* render_finished_sems;
    VkFence in_flight_fence;

    // Shared Heightmap SSBO
    uint32_t grid_w;
    uint32_t grid_h;
    VkBuffer ssbo_buffer;
    VkDeviceMemory ssbo_memory;
    void* ssbo_mapped_ptr;

    // Descriptoers & graphics pipeline
    VkDescriptorSetLayout desc_set_layout;
    VkDescriptorPool desc_pool;
    VkDescriptorSet desc_set;
    VkPipelineLayout pipeline_layout;
    VkPipeline graphics_pipeline;
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

static int create_swapchain(int width, int height) {
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx.physical_device, ctx.surface, &caps);

    // Pick surface format (prefer 8-bit BGRA SRGB)
    uint32_t fmt_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(ctx.physical_device, ctx.surface, &fmt_count, NULL);
    VkSurfaceFormatKHR *formats = malloc(sizeof(VkSurfaceFormatKHR)*fmt_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(ctx.physical_device, ctx.surface, &fmt_count, formats);

    VkSurfaceFormatKHR chosen_fmt = formats[0];
    for (uint32_t i=0; i<fmt_count; i++) {
        if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB
         && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            chosen_fmt = formats[i];
            break;
        }
    }
    free(formats);

    ctx.swapchain_format = chosen_fmt.format;
    ctx.swapchain_extent = (VkExtent2D){ (uint32_t)width, (uint32_t)height };

    uint32_t img_count = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && img_count > caps.maxImageCount) {
        img_count = caps.maxImageCount;
    }

    VkSwapchainCreateInfoKHR sc_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = ctx.surface,
        .minImageCount = img_count,
        .imageFormat = ctx.swapchain_format,
        .imageColorSpace = chosen_fmt.colorSpace,
        .imageExtent = ctx.swapchain_extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = caps.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR, // guaranteeing v-sync
        .clipped = VK_TRUE
    };

    if (vkCreateSwapchainKHR(ctx.device, &sc_info, NULL, &ctx.swapchain) != VK_SUCCESS) return -1;

    vkGetSwapchainImagesKHR(ctx.device, ctx.swapchain, &ctx.image_count, NULL);
    ctx.images = malloc(sizeof(VkImage) * ctx.image_count);
    ctx.image_views = malloc(sizeof(VkImageView) * ctx.image_count);
    vkGetSwapchainImagesKHR(ctx.device, ctx.swapchain, &ctx.image_count, ctx.images);

    for (uint32_t i=0; i<ctx.image_count; i++) {
        VkImageViewCreateInfo view_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = ctx.images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = ctx.swapchain_format,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };
        if (vkCreateImageView(ctx.device, &view_info, NULL, &ctx.image_views[i]) != VK_SUCCESS) return -1;
    }

    return 0;
}

static int create_commands_and_sync(void) {
    VkCommandPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = ctx.queue_family_index
    };
    if (vkCreateCommandPool(ctx.device, &pool_info, NULL, &ctx.cmd_pool) != VK_SUCCESS) return -1;

    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = ctx.cmd_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    if (vkAllocateCommandBuffers(ctx.device, &alloc_info, &ctx.cmd_buf) != VK_SUCCESS) return -1;

    VkSemaphoreCreateInfo sem_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };
    VkFenceCreateInfo fence_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT // start signaled so frame 1 doesn't wait forever
    };

    if (vkCreateSemaphore(ctx.device, &sem_info, NULL, &ctx.image_available_sem) != VK_SUCCESS
     || vkCreateFence(ctx.device, &fence_info, NULL, &ctx.in_flight_fence) != VK_SUCCESS) return -1;

    ctx.render_finished_sems = malloc(sizeof(VkSemaphore) * ctx.image_count);
    for (uint32_t i=0; i<ctx.image_count; i++) {
        if (vkCreateSemaphore(ctx.device, &sem_info, NULL, &ctx.render_finished_sems[i]) != VK_SUCCESS) return -1;
    }

    return 0;
}

static uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(ctx.physical_device, &mem_props);

    for (uint32_t i=0; i<mem_props.memoryTypeCount; i++) {
        if ((type_filter & (1 << i))
         && (mem_props.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    fprintf(stderr, "Failed to find suitable memory type!\n");
    exit(1);
}

static VkShaderModule load_shader_module(const char* path) {
    FILE* file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open shader file:%s\n", path);
        exit(1);
    }
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);

    uint32_t* code = malloc(size);
    if (fread(code, 1, size, file) != (size_t)size) {
        fprintf(stderr, "Failed to read shader file:%s\n", path);
        exit(1);
    }
    fclose(file);

    VkShaderModuleCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = (size_t)size,
        .pCode = code
    };

    VkShaderModule module;
    if (vkCreateShaderModule(ctx.device, &info, NULL, &module) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create shader module:%s\n", path);
        exit(1);
    }
    free(code);
    return module;
}

static int create_descriptors_and_pipeline(void) {
    // Descriptor set layout (binding 0 = SSBO visible to fragment shader)
    VkDescriptorSetLayoutBinding binding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
    };
    VkDescriptorSetLayoutCreateInfo layout_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &binding
    };
    if (vkCreateDescriptorSetLayout(ctx.device, &layout_info, NULL, &ctx.desc_set_layout) != VK_SUCCESS) return -1;

    // Descriptor pool & descriptor set allocation
    VkDescriptorPoolSize pool_size = {
        .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptorCount = 1
    };
    VkDescriptorPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 1,
        .poolSizeCount = 1,
        .pPoolSizes = &pool_size
    };
    if (vkCreateDescriptorPool(ctx.device, &pool_info, NULL, &ctx.desc_pool) != VK_SUCCESS) return -1;

    VkDescriptorSetAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = ctx.desc_pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &ctx.desc_set_layout
    };
    if (vkAllocateDescriptorSets(ctx.device, &alloc_info, &ctx.desc_set) != VK_SUCCESS) return -1;

    // Pipeline layout (descriptor set + 8-byte push constants for grid_w, grid_h)
    VkPushConstantRange push_range = {
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .offset = 0,
        .size = sizeof(uint32_t)*2
    };
    VkPipelineLayoutCreateInfo pipe_layout_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &ctx.desc_set_layout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &push_range
    };
    if (vkCreatePipelineLayout(ctx.device, &pipe_layout_info, NULL, &ctx.pipeline_layout) != VK_SUCCESS) return -1;

    // Load SPIR-V shaders
    VkShaderModule vert_mod = load_shader_module("build/map.vert.spv");
    VkShaderModule frag_mod = load_shader_module("build/map.frag.spv");

    VkPipelineShaderStageCreateInfo stages[2] = {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vert_mod,
            .pName = "main"
        }, {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = frag_mod,
            .pName = "main"
        }
    };

    // Fixed-function state (empty vertex input, fullscreen triangle topology)
    VkPipelineVertexInputStateCreateInfo vertex_input = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
    };
    VkPipelineInputAssemblyStateCreateInfo input_assembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
    };
    VkPipelineViewportStateCreateInfo viewport_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1
    };
    VkPipelineRasterizationStateCreateInfo rasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .lineWidth = 1.0f
    };
    VkPipelineMultisampleStateCreateInfo multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
    };
    VkPipelineColorBlendAttachmentState blend_attachment = {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | 
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        .blendEnable = VK_FALSE
    };
    VkPipelineColorBlendStateCreateInfo color_blending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &blend_attachment
    };

    VkDynamicState dyn_states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamic_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = 2,
        .pDynamicStates = dyn_states
    };

    // Vulkan 1.3 dynamic rendering format link
    VkPipelineRenderingCreateInfo rendering_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &ctx.swapchain_format
    };
    VkGraphicsPipelineCreateInfo pipe_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &rendering_info,
        .stageCount = 2,
        .pStages = stages,
        .pVertexInputState = &vertex_input,
        .pInputAssemblyState = &input_assembly,
        .pViewportState = &viewport_state,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pColorBlendState = &color_blending,
        .pDynamicState = &dynamic_state,
        .layout = ctx.pipeline_layout
    };

    if (vkCreateGraphicsPipelines(ctx.device, VK_NULL_HANDLE, 1, &pipe_info, NULL, &ctx.graphics_pipeline) != VK_SUCCESS) return -1;

    // Shader modules are no longer needed once compiled into the pipeline
    vkDestroyShaderModule(ctx.device, vert_mod, NULL);
    vkDestroyShaderModule(ctx.device, frag_mod, NULL);
    return 0;
}

// Called from Fortran - allocates the shared buffer and returns the raw pointer
void* vk_allocate_heightmap(int grid_w, int grid_h) {
    ctx.grid_w = (uint32_t)grid_w;
    ctx.grid_h = (uint32_t)grid_h;
    VkDeviceSize buffer_size = sizeof(float) * ctx.grid_w * ctx.grid_h;

    VkBufferCreateInfo buf_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = buffer_size,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    if (vkCreateBuffer(ctx.device, &buf_info, NULL, &ctx.ssbo_buffer) != VK_SUCCESS) return NULL;

    VkMemoryRequirements mem_reqs;
    vkGetBufferMemoryRequirements(ctx.device, ctx.ssbo_buffer, &mem_reqs);

    VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mem_reqs.size,
        .memoryTypeIndex = find_memory_type(
            mem_reqs.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        )
    };

    if (vkAllocateMemory(ctx.device, &alloc_info, NULL, &ctx.ssbo_memory) != VK_SUCCESS) return NULL;
    vkBindBufferMemory(ctx.device, ctx.ssbo_buffer, ctx.ssbo_memory, 0);

    // Map persistently for the lifetime of the program
    vkMapMemory(ctx.device, ctx.ssbo_memory, 0, buffer_size, 0, &ctx.ssbo_mapped_ptr);

    // Bind this buffer to Set 0, Binding 0 in our Descriptor Set
    VkDescriptorBufferInfo desc_buf_info = {
        .buffer = ctx.ssbo_buffer,
        .offset = 0,
        .range = buffer_size
    };
    VkWriteDescriptorSet write_desc = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = ctx.desc_set,
        .dstBinding = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pBufferInfo = &desc_buf_info
    };
    vkUpdateDescriptorSets(ctx.device, 1, &write_desc, 0, NULL);

    return ctx.ssbo_mapped_ptr;
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
    if (create_swapchain(width, height) != 0) return -1;
    if (create_commands_and_sync() != 0) return -1;
    if (create_descriptors_and_pipeline() != 0) return -1;

    printf("Swapchain and command pipeline ready (%u images)\n", ctx.image_count);
    return 0;
}

void vk_draw_frame(void) {
    // Wait for teh previous frame to finish on the GPU
    vkWaitForFences(ctx.device, 1, &ctx.in_flight_fence, VK_TRUE, UINT64_MAX);
    vkResetFences(ctx.device, 1, &ctx.in_flight_fence);

    // Aquire the next drawable image index from the swapchain
    uint32_t img_idx = 0;
    vkAcquireNextImageKHR(ctx.device, ctx.swapchain, UINT64_MAX, ctx.image_available_sem, VK_NULL_HANDLE, &img_idx);

    // Reset and begin recording the command buffer
    vkResetCommandBuffer(ctx.cmd_buf, 0);
    VkCommandBufferBeginInfo begin_info = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    vkBeginCommandBuffer(ctx.cmd_buf, &begin_info);

    // Transition swapchain image: UNDEFINED -> COLOR_ATTACHMENT_OPTIMAL
    VkImageMemoryBarrier2 barrier_to_color = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
        .srcAccessMask = 0,
        .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .image = ctx.images[img_idx],
        .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
    };
    VkDependencyInfo dep_to_color = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier_to_color
    };
    vkCmdPipelineBarrier2(ctx.cmd_buf, &dep_to_color);

    // Begin dynamic rendering (clear screen to light blue)
    VkClearValue clear_color = { .color = {{ 0.15f, 0.35f, 0.65f, 1.0f }} };
    VkRenderingAttachmentInfo color_attachment = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = ctx.image_views[img_idx],
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = clear_color
    };
    VkRenderingInfo render_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = { {0,0}, ctx.swapchain_extent },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment
    };

    vkCmdBeginRendering(ctx.cmd_buf, &render_info);

    if (ctx.ssbo_buffer != VK_NULL_HANDLE) {
        vkCmdBindPipeline(ctx.cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx.graphics_pipeline);
        
        VkViewport viewport = {
            .x = 0.0f, .y = 0.0f,
            .width = (float)ctx.swapchain_extent.width,
            .height = (float)ctx.swapchain_extent.height,
            .minDepth = 0.0f, .maxDepth = 1.0f
        };
        vkCmdSetViewport(ctx.cmd_buf, 0, 1, &viewport);

        VkRect2D scissor = { .offset = {0,0}, .extent = ctx.swapchain_extent };
        vkCmdSetScissor(ctx.cmd_buf, 0, 1, &scissor);

        vkCmdBindDescriptorSets(ctx.cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                ctx.pipeline_layout, 0, 1, &ctx.desc_set, 0, NULL);

        uint32_t push_constants[2] = { ctx.grid_w, ctx.grid_h };
        vkCmdPushConstants(ctx.cmd_buf, ctx.pipeline_layout,
                VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(push_constants), push_constants);

        // Draw our 3-vertex procedural fullscreen triangle
        vkCmdDraw(ctx.cmd_buf, 3, 1, 0, 0);
    }

    vkCmdEndRendering(ctx.cmd_buf);

    // Transition swapchain image: COLOR_ATTACHMENT_OPTIMAL -> PRESENT_SRC_KHR
    VkImageMemoryBarrier2 barrier_to_present = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
        .dstAccessMask = 0,
        .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .image = ctx.images[img_idx],
        .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
    };
    VkDependencyInfo dep_to_present = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier_to_present
    };
    vkCmdPipelineBarrier2(ctx.cmd_buf, &dep_to_present);

    vkEndCommandBuffer(ctx.cmd_buf);

    // Submit command buffer to queue
    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &ctx.image_available_sem,
        .pWaitDstStageMask = &wait_stage,
        .commandBufferCount = 1,
        .pCommandBuffers = &ctx.cmd_buf,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &ctx.render_finished_sems[img_idx]
    };
    vkQueueSubmit(ctx.queue, 1, &submit_info, ctx.in_flight_fence);

    // Present the rendered image to  the window
    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &ctx.render_finished_sems[img_idx],
        .swapchainCount = 1,
        .pSwapchains = &ctx.swapchain,
        .pImageIndices = &img_idx
    };
    vkQueuePresentKHR(ctx.queue, &present_info);
}


int vk_should_close(void) {
    glfwPollEvents();
    return glfwWindowShouldClose(ctx.window);
}

void vk_cleanup(void) {
    vkDeviceWaitIdle(ctx.device);

    // Deallocate the shared buffer
    if (ctx.ssbo_buffer != VK_NULL_HANDLE) {
        vkUnmapMemory(ctx.device, ctx.ssbo_memory);
        vkDestroyBuffer(ctx.device, ctx.ssbo_buffer, NULL);
        vkFreeMemory(ctx.device, ctx.ssbo_memory, NULL);
    }

    vkDestroyPipeline(ctx.device, ctx.graphics_pipeline, NULL);
    vkDestroyPipelineLayout(ctx.device, ctx.pipeline_layout, NULL);
    vkDestroyDescriptorPool(ctx.device, ctx.desc_pool, NULL);
    vkDestroyDescriptorSetLayout(ctx.device, ctx.desc_set_layout, NULL);

    vkDestroySemaphore(ctx.device, ctx.image_available_sem, NULL);
    vkDestroyFence(ctx.device, ctx.in_flight_fence, NULL);
    vkDestroyCommandPool(ctx.device, ctx.cmd_pool, NULL);

    for (uint32_t i=0; i<ctx.image_count; i++) {
        vkDestroyImageView(ctx.device, ctx.image_views[i], NULL);
    vkDestroySemaphore(ctx.device, ctx.render_finished_sems[i], NULL);
    }

    free(ctx.images);
    free(ctx.image_views);
    free(ctx.render_finished_sems);

    vkDestroySwapchainKHR(ctx.device, ctx.swapchain, NULL);

    vkDestroyDevice(ctx.device, NULL);
    vkDestroySurfaceKHR(ctx.instance, ctx.surface, NULL);
    vkDestroyInstance(ctx.instance, NULL);

    glfwDestroyWindow(ctx.window);
    glfwTerminate();

    printf("Vulkan resources cleaned up.\n");
}
