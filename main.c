#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define VOLK_IMPLEMENTATION
#define VK_NO_PROTOTYPES
#include "external/volk/volk.h"

#define GGLFW_INCLUDE_VULKAN
#define GLFW_EXPOSE_NATIVE_WAYLAND
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#define u32 uint32_t
#define VK_CHECK(call) \
do { \
  VkResult result_ = call; \
  assert(result_ == VK_SUCCESS); \
} while (0)
#define ARRAYSIZE(array) (sizeof(array) / sizeof((array)[0]))

int main() {
	volkInitialize() ;
    int rc = glfwInit();
    assert(rc);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow *window = glfwCreateWindow(800, 600, "niagara", 0, 0);
    assert(window);
    int windowWidth = 0, windowHeight = 0;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    
    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .apiVersion = VK_API_VERSION_1_3,
    };
    
    VkInstanceCreateInfo createInfo = {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo = &appInfo,
    };
    
  #ifdef _DEBUG
    const char *debugLayers[] = {"VK_LAYER_KHRONOS_validation"};
    createInfo.ppEnabledLayerNames = debugLayers;
    createInfo.enabledLayerCount = ARRAYSIZE(debugLayers);
  #endif
  
    const char *extensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
  #ifdef VK_USE_PLATFORM_WAYLAND_KHR
        VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
  #endif
  #ifndef NDEBUG
        VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
  #endif
    };
    
    createInfo.ppEnabledExtensionNames = extensions;
    createInfo.enabledExtensionCount = ARRAYSIZE(extensions);
    
    VkInstance instance;
    VK_CHECK(vkCreateInstance(&createInfo, 0, &instance));
   volkLoadInstance(instance); 

    VkPhysicalDevice physicalDevices[8];
    u32 physicalDeviceCount = ARRAYSIZE(physicalDevices);
    VK_CHECK(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices));
    
    VkPhysicalDevice selectedPhysicalDevice = VK_NULL_HANDLE,
                     discrete = VK_NULL_HANDLE, fallback = VK_NULL_HANDLE;
    
    for (u32 i = 0; i < physicalDeviceCount; ++i) {
      VkPhysicalDeviceProperties props = {0};
      vkGetPhysicalDeviceProperties(physicalDevices[i], &props);
      printf("GPU%d: %s\n", i, props.deviceName);
      discrete = (!discrete && props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
              ? physicalDevices[i] : discrete;
      fallback = (!fallback) ? physicalDevices[i] : fallback;
    }
    
    selectedPhysicalDevice = discrete ? discrete : fallback;
    if (selectedPhysicalDevice) {
      VkPhysicalDeviceProperties props = {0};
      vkGetPhysicalDeviceProperties(selectedPhysicalDevice, &props);
      printf("Selected GPU: %s\n", props.deviceName);
    } else {
      printf("No suitable GPU found\n");
      exit(1);
    }
    
    u32 queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(selectedPhysicalDevice, &queueFamilyCount, NULL);
    VkQueueFamilyProperties *queueFamilies = malloc(queueFamilyCount * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(selectedPhysicalDevice, &queueFamilyCount, queueFamilies);
    
    u32 queuefamilyIndex = UINT32_MAX;
    for (u32 i = 0; i < queueFamilyCount; ++i) {
      if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        queuefamilyIndex = i;
        break;
      }
    }
    
    assert(queuefamilyIndex != UINT32_MAX && "No suitable queue family found");
    float queuePriority = 1.0f;
    
    VkDeviceQueueCreateInfo queueCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queuefamilyIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority,
    };
    
    VkPhysicalDeviceFeatures deviceFeatures = {0};
    const char *deviceExtensions[] = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
      VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME 
    };
  
    VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeature = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR,
      .dynamicRendering = VK_TRUE, 
    };
  
    VkDeviceCreateInfo deviceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &dynamicRenderingFeature,  // This should be here, not before declaration
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueCreateInfo,
        .enabledExtensionCount = ARRAYSIZE(deviceExtensions),
        .ppEnabledExtensionNames = deviceExtensions,
        .pEnabledFeatures = &deviceFeatures,
    };
    
    VkDevice device;
    VK_CHECK(vkCreateDevice(selectedPhysicalDevice, &deviceCreateInfo, 0, &device));
    // surface creation
    
    VkSurfaceKHR surface = 0;
	VK_CHECK(glfwCreateWindowSurface(instance, window, 0, &surface));
    
    VkBool32 presentSupported = 0;
    VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(selectedPhysicalDevice, queuefamilyIndex, surface, &presentSupported));
    assert(presentSupported);
    
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(selectedPhysicalDevice, surface, &surfaceCapabilities));
    
    u32 formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(selectedPhysicalDevice, surface, &formatCount, NULL);
    VkSurfaceFormatKHR *formats = malloc(formatCount * sizeof(VkSurfaceFormatKHR));
    vkGetPhysicalDeviceSurfaceFormatsKHR(selectedPhysicalDevice, surface, &formatCount, formats);
    
    VkSwapchainKHR swapchain;
    VkSwapchainCreateInfoKHR swapchaincreateinfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = surfaceCapabilities.minImageCount,
        .imageFormat = formats[0].format,
        .imageColorSpace = formats[0].colorSpace,
        .imageExtent = {.width = windowWidth, .height = windowHeight},
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
        .clipped = VK_TRUE,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &queuefamilyIndex,
    };
    
    VK_CHECK(vkCreateSwapchainKHR(device, &swapchaincreateinfo, 0, &swapchain));
    
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderCompleteSemaphore;
    VkSemaphoreCreateInfo semInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    VK_CHECK(vkCreateSemaphore(device, &semInfo, 0, &imageAvailableSemaphore));
    VK_CHECK(vkCreateSemaphore(device, &semInfo, 0, &renderCompleteSemaphore));
    
    VkQueue queue;
    vkGetDeviceQueue(device, queuefamilyIndex, 0, &queue);
    
    VkCommandPoolCreateInfo commandPoolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = queuefamilyIndex,
        .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
    };
    
    VkCommandPool commandpool;
    VK_CHECK(vkCreateCommandPool(device, &commandPoolInfo, NULL, &commandpool));
    
    u32 swapchainimageCount = 0;
    VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &swapchainimageCount, NULL));
    VkImage *swapchainImages = malloc(swapchainimageCount * sizeof(VkImage));
    VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &swapchainimageCount, swapchainImages));
    
    VkImageView *swapchainImageViews = malloc(swapchainimageCount * sizeof(VkImageView));
    for (u32 i = 0; i < swapchainimageCount; ++i) {
      VkImageViewCreateInfo imageViewInfo = {
          .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
          .image = swapchainImages[i],
          .viewType = VK_IMAGE_VIEW_TYPE_2D,
          .format = swapchaincreateinfo.imageFormat,
          .components = {
              .r = VK_COMPONENT_SWIZZLE_IDENTITY,
              .g = VK_COMPONENT_SWIZZLE_IDENTITY,
              .b = VK_COMPONENT_SWIZZLE_IDENTITY,
              .a = VK_COMPONENT_SWIZZLE_IDENTITY,
          },
          .subresourceRange = {
              .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
              .baseMipLevel = 0,
              .levelCount = 1,
              .baseArrayLayer = 0,
              .layerCount = 1,
          },
      };
      VK_CHECK(vkCreateImageView(device, &imageViewInfo, NULL, &swapchainImageViews[i]));
    }
    
  
    
    VkShaderModule triangleVS;
    {
      FILE *file = fopen("shaders/tri.vert.spv", "rb");
      assert(file);
      fseek(file, 0, SEEK_END);
      long length = ftell(file);
      assert(length >= 0);
      fseek(file, 0, SEEK_SET);
      char *buffer = (char *)malloc(length);
      assert(buffer);
      size_t rc = fread(buffer, 1, length, file);
      assert(rc == (size_t)length);
      fclose(file);
      VkShaderModuleCreateInfo createInfo = {0};
      createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
      createInfo.codeSize = length;
      createInfo.pCode = (const uint32_t *)buffer;
      VK_CHECK(vkCreateShaderModule(device, &createInfo, NULL, &triangleVS));
      free(buffer);
    }
    
    VkShaderModule triangleFS;
    {
      FILE *file = fopen("shaders/tri.frag.spv", "rb");
      assert(file);
      fseek(file, 0, SEEK_END);
      long length = ftell(file);
      assert(length >= 0);
      fseek(file, 0, SEEK_SET);
      char *buffer = (char *)malloc(length);
      assert(buffer);
      size_t rc = fread(buffer, 1, length, file);
      assert(rc == (size_t)length);
      fclose(file);
      VkShaderModuleCreateInfo createInfo = {0};  // Changed from {1} to {0}
      createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
      createInfo.codeSize = length;
      createInfo.pCode = (const uint32_t *)buffer;
      VK_CHECK(vkCreateShaderModule(device, &createInfo, NULL, &triangleFS));
      free(buffer);
    }
    
    VkPipelineLayout pipelinelayout;
    VkPipelineLayoutCreateInfo pipelinecreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    };
    VK_CHECK(vkCreatePipelineLayout(device, &pipelinecreateInfo, 0, &pipelinelayout));
  
    VkPipelineRenderingCreateInfoKHR pipelineRenderingInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
      .colorAttachmentCount = 1,
      .pColorAttachmentFormats = &swapchaincreateinfo.imageFormat,
    };
    
    VkGraphicsPipelineCreateInfo pipelineinfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &pipelineRenderingInfo,
        // REMOVED: .renderPass = renderPass,
    };
    
    VkPipelineShaderStageCreateInfo stages[2] = {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = triangleVS,
            .pName = "main",
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = triangleFS,
            .pName = "main",
        },
    };
    
    pipelineinfo.stageCount = ARRAYSIZE(stages);
    pipelineinfo.pStages = stages;
    
    VkPipelineVertexInputStateCreateInfo vertexInput = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    };
    pipelineinfo.pVertexInputState = &vertexInput;
    
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,  // Change this!
  };
    pipelineinfo.pInputAssemblyState = &inputAssembly;
    
    VkPipelineViewportStateCreateInfo viewportState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
    };
    pipelineinfo.pViewportState = &viewportState;
    
    VkPipelineRasterizationStateCreateInfo rasterizationState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .lineWidth = 1.f,
    };
    pipelineinfo.pRasterizationState = &rasterizationState;
    
    VkPipelineMultisampleStateCreateInfo multisampleState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };
    pipelineinfo.pMultisampleState = &multisampleState;
    
    VkPipelineDepthStencilStateCreateInfo depthStencilState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    };
    pipelineinfo.pDepthStencilState = &depthStencilState;
    
    VkPipelineColorBlendAttachmentState colorAttachmentState = {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };
    
    VkPipelineColorBlendStateCreateInfo colorBlendState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &colorAttachmentState,
    };
    pipelineinfo.pColorBlendState = &colorBlendState;
    
    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = ARRAYSIZE(dynamicStates),
        .pDynamicStates = dynamicStates,
    };
    pipelineinfo.pDynamicState = &dynamicState;
    pipelineinfo.layout = pipelinelayout;
    
    VkPipeline pipeline = 0;
    VkPipelineCache pipelineCache = 0;
    VK_CHECK(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineinfo, 0, &pipeline));
    
    VkCommandBuffer commandBuffer; 
    while (!glfwWindowShouldClose(window)) {
      glfwPollEvents();
  
      u32 imageIndex = 0;
      
      VK_CHECK(vkAcquireNextImageKHR(device, swapchain, ~0ull, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex));
      VK_CHECK(vkResetCommandPool(device, commandpool, 0));
      
      VkCommandBufferAllocateInfo commandBufferInfo = {
          .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
          .commandPool = commandpool,
          .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
          .commandBufferCount = 1,
      };
      
      VK_CHECK(vkAllocateCommandBuffers(device, &commandBufferInfo, &commandBuffer));
      
      VkCommandBufferBeginInfo begininfo = {
          .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
          .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
      };
      
      VK_CHECK(vkBeginCommandBuffer(commandBuffer, &begininfo));

// Transition swapchain image from UNDEFINED to COLOR_ATTACHMENT_OPTIMAL
VkImageMemoryBarrier colorAttachmentBarrier = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
    .srcAccessMask = 0,
    .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .image = swapchainImages[imageIndex],
    .subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1
    }
};

vkCmdPipelineBarrier(
    commandBuffer,
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    0,
    0, NULL,
    0, NULL,
    1, &colorAttachmentBarrier
);


VkClearValue clearValue = {.color = {{0.1f, 0.0f, 0.2f, 1.0f}}}; 


VkRenderingInfoKHR renderInfo = {
    .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
    .renderArea = {.offset = {0, 0}, .extent = {windowWidth, windowHeight}},
    .layerCount = 1,
    .colorAttachmentCount = 1,
    .pColorAttachments = &(VkRenderingAttachmentInfoKHR){ 
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
        .imageView = swapchainImageViews[imageIndex],     
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,   
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE, 
        .clearValue = clearValue,                
    },
};

vkCmdBeginRenderingKHR(commandBuffer, &renderInfo);

      VkViewport viewport = {
          .x = 0.0f,
          .y = 0.0f,
          .width = (float)windowWidth,
          .height = (float)windowHeight,
          .minDepth = 0.0f,
          .maxDepth = 1.0f,
      };
      vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
  
      VkRect2D scissor = {
          .offset = {0, 0},
          .extent = {windowWidth, windowHeight},
      };
      vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
  
      vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
      vkCmdDraw(commandBuffer, 18, 1, 0, 0);  
  
      vkCmdEndRenderingKHR(commandBuffer);

      // ADD THIS: Transition swapchain image to present layout
      VkImageMemoryBarrier presentBarrier = {
          .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
          .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
          .dstAccessMask = 0,
          .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
          .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
          .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
          .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
          .image = swapchainImages[imageIndex],
          .subresourceRange = {
              .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
              .baseMipLevel = 0,
              .levelCount = 1,
              .baseArrayLayer = 0,
              .layerCount = 1
          }
      };
      
      vkCmdPipelineBarrier(
          commandBuffer,
          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
          VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
          0,
          0, NULL,
          0, NULL,
          1, &presentBarrier
      );
      
      VK_CHECK(vkEndCommandBuffer(commandBuffer));
      
      
      VkSubmitInfo submitInfo = {
          .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
          .waitSemaphoreCount = 1,
          .pWaitSemaphores = &imageAvailableSemaphore,
          .pWaitDstStageMask = (VkPipelineStageFlags[]){VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
          .commandBufferCount = 1,
          .pCommandBuffers = &commandBuffer,
          .signalSemaphoreCount = 1,
          .pSignalSemaphores = &renderCompleteSemaphore,
      };
      
      VK_CHECK(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
      
      VkPresentInfoKHR presentInfo = {
          .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
          .waitSemaphoreCount = 1,
          .pWaitSemaphores = &renderCompleteSemaphore,
          .swapchainCount = 1,
          .pSwapchains = &swapchain,
          .pImageIndices = &imageIndex,
      };
      
      VK_CHECK(vkQueuePresentKHR(queue, &presentInfo));
      VK_CHECK(vkDeviceWaitIdle(device)); 
    }
    
    vkDeviceWaitIdle(device);
    vkFreeCommandBuffers(device, commandpool, 1, &commandBuffer);
    vkDestroyCommandPool(device, commandpool, 0);
    
    // Removed Framebuffer destruction
    for (uint32_t i = 0; i < swapchainimageCount; ++i) {
      vkDestroyImageView(device, swapchainImageViews[i], 0);
    }
    
    vkDestroyPipeline(device, pipeline, 0);
    vkDestroyPipelineLayout(device, pipelinelayout, 0);
    vkDestroyShaderModule(device, triangleFS, 0);
    vkDestroyShaderModule(device, triangleVS, 0);
    // Removed vkDestroyRenderPass(device, renderPass, 0);
    
    vkDestroySemaphore(device, renderCompleteSemaphore, 0);
    vkDestroySemaphore(device, imageAvailableSemaphore, 0);
    vkDestroySwapchainKHR(device, swapchain, 0);
    vkDestroySurfaceKHR(instance, surface, 0);
    glfwDestroyWindow(window);
    vkDestroyDevice(device, 0);
    vkDestroyInstance(instance, 0);
    
    free(swapchainImages);
    free(swapchainImageViews);
    free(formats);
    free(queueFamilies);
    
    return 0;
  }
