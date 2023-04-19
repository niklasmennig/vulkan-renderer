#include <iostream>
#include <vector>

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include "vulkan_application.h"

int main()
{
    // initialize glfw
    glfwInit();

    VulkanApplication app;
    app.setup();
    app.run();

    app.cleanup();

    return 0;
}