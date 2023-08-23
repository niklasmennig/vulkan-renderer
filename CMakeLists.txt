cmake_minimum_required(VERSION 3.5)

project(vulkanrenderer)
set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

set(CMAKE_CXX_FLAGS -MD)

set(GLSLC_EXE "C:/VulkanSDK/1.3.250.0/Bin/glslc")
set(SHADER_DIR "${PROJECT_SOURCE_DIR}/shaders")

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

if (MSVC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:MSVCRT")
endif()

include_directories("includes/" ${GLFW3_INCLUDE_DIR} "src/imgui" ${VULKAN_INCLUDE_DIR})

add_subdirectory(src)

add_executable(renderer ${SRC_RENDERER})
set_property(TARGET renderer PROPERTY CXX_STANDARD 17)

message(TEST:"${Vulkan_glslang_LIBRARY}")

target_compile_definitions(renderer PRIVATE GLSLC_EXE="${GLSLC_EXE}" SHADER_DIR="${SHADER_DIR}")

target_link_libraries(renderer ${GLFW3_LIBRARY} Vulkan::Vulkan imgui)