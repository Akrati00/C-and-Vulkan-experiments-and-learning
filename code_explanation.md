#include <assert.h>      // For assert() - crashes program if condition is false
#include <stdbool.h>     // For bool, true, false
#include <stdint.h>      // For uint32_t, etc.
#include <stdio.h>       // For printf, fopen, etc.
#include <stdlib.h>      // For malloc, free, exit
#include <vulkan/vulkan.h>         // Main Vulkan API
#include <vulkan/vulkan_core.h>    // Core Vulkan types
#define GGLFW_INCLUDE_VULKAN       // Tell GLFW to include Vulkan
#define GLFW_EXPOSE_NATIVE_WAYLAND // Enable Wayland support
#include <GLFW/glfw3.h>            // Window management
#include <GLFW/glfw3native.h>      // Platform-specific functions
#include <vulkan/vulkan_wayland.h> // Wayland surface creation
