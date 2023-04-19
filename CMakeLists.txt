cmake_minimum_required(VERSION 3.0)

project(vulkanrenderer)
set (CMAKE_VERBOSE_MAKEFILE ON)

# add include directories
include_directories(
    # Vulkan SDK
    "C:/VulkanSDK/1.3.239.0/Include"
    # glfw
    "C:/Users/niloe/Documents/Libraries/glfw-3.3.8.bin.WIN64/include"
    # glm
    "C:/Users/niloe/Documents/Libraries/glm"
)

# set lib directories
set (LIB_VULKAN "C:/VulkanSDK/1.3.239.0/Lib")
set (LIB_GLFW "C:/Users/niloe/Documents/Libraries/glfw-3.3.8.bin.WIN64/lib-vc2022")

if (MSVC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:MSVCRT")
endif()

add_subdirectory(src)

add_executable(renderer ${SRC_RENDERER})
set_property(TARGET renderer PROPERTY CXX_STANDARD 17)
target_link_libraries(renderer "${LIB_VULKAN}/vulkan-1.lib" "${LIB_GLFW}/glfw3.lib")