add_subdirectory(imgui)

set(SRC_RENDERER
    ${SRC_IMGUI}
    src/mikktspace/mikktspace.c
    src/image.cpp
    src/loaders/geometry.cpp
    #src/loaders/geometry_obj.cpp -- deprecated
    src/loaders/geometry_gltf.cpp
    src/loaders/image.cpp
    src/loaders/scene.cpp
    src/buffer.cpp
    src/device.cpp
    src/pipeline_builder.cpp
    src/vulkan_application.cpp
    src/main.cpp
PARENT_SCOPE)