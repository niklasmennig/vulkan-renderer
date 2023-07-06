cmake_minimum_required(VERSION 3.0)

project(vulkanrenderer)
set (CMAKE_VERBOSE_MAKEFILE ON)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

find_package(Vulkan REQUIRED glslang)
find_package(glfw3 REQUIRED)

if (MSVC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:MSVCRT")
endif()

include_directories("includes/" ${GLFW3_INCLUDE_DIR} "src/imgui" ${VULKAN_INCLUDE_DIR})

add_subdirectory(src)

add_executable(renderer ${SRC_RENDERER})
set_property(TARGET renderer PROPERTY CXX_STANDARD 17)

message(TEST:"${Vulkan_glslang_LIBRARY}")

target_link_libraries(renderer ${GLFW3_LIBRARY} Vulkan::Vulkan Vulkan::glslang imgui)