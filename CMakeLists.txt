cmake_minimum_required(VERSION 3.5)

project(vulkanrenderer)
set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

set(CMAKE_CXX_FLAGS -MD)

set(SHADER_DIR "${PROJECT_SOURCE_DIR}/shaders")

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

find_package(Vulkan REQUIRED)
find_package(GLFW3 REQUIRED)

if (MSVC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:MSVCRT")
endif()


add_subdirectory(src)