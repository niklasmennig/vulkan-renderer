add_subdirectory(imgui)

add_executable(renderer main.cpp)

target_sources(renderer PRIVATE
    shader_compiler.cpp
    core/memory.cpp
    core/device.cpp
    core/buffer.cpp
    core/image.cpp
    loaders/mikktspace/mikktspace.c
    loaders/geometry.cpp
    loaders/geometry_gltf.cpp
    loaders/image.cpp
    loaders/scene.cpp
    loaders/environment.cpp
    processors/gltf/gltf_processor.cpp
    pipeline/raytracing/pipeline_stage.cpp
    pipeline/raytracing/pipeline_stage_simple.cpp
    pipeline/raytracing/pipeline_builder.cpp
    pipeline/processing/compute_shader.cpp
    pipeline/processing/pipeline_stage_simple.cpp
    pipeline/processing/pipeline_stage_oidn.cpp
    pipeline/processing/pipeline_builder.cpp
    ui.cpp
    vulkan_application.cpp
    main.cpp
)

set (OpenImageDenoise_INCLUDE_DIR ${CMAKE_SOURCE_DIR} CACHE PATH "Path to Open Image Denoise Include Directory")
set (OpenImageDenoise_LIBRARY ${CMAKE_SOURCE_DIR} CACHE FILEPATH "Path to Open Image Denoise Library")

target_include_directories(renderer PRIVATE 
    ${PROJECT_SOURCE_DIR}/includes/
    ${VULKAN_INCLUDE_DIR} 
    ${GLFW3_INCLUDE_DIR} 
    ${OpenImageDenoise_INCLUDE_DIR} 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui/
)

set_property(TARGET renderer PROPERTY CXX_STANDARD 17)
target_compile_definitions(renderer PRIVATE GLSLC_EXECUTABLE="${Vulkan_GLSLC_EXECUTABLE}" SHADER_DIR="${SHADER_DIR}")
target_link_libraries(renderer ${GLFW3_LIBRARY} Vulkan::Vulkan imgui ${OpenImageDenoise_LIBRARY})