#pragma once

#include "pipeline/processing/pipeline_stage.h"
#include "pipeline/processing/compute_shader.h"

#include "core/vulkan.h"
#include "core/image.h"

struct ProcessingPipelineStageRestir : ProcessingPipelineStage {
    Buffer restir_buffers[2];
    ComputeShader *compute_shader_initial_temporal, *compute_shader_spatial;

    VkAccelerationStructureKHR acceleration_structure;

    Buffer *indices, *vertices, *normals, *texcoords, *tangents, *mesh_data_offsets, *mesh_offset_indices, *texture_indices, *material_parameters, *lights, *previous_camera_data;
    std::vector<Image>* loaded_textures;

    ProcessingPipelineStageRestir(VkAccelerationStructureKHR acceleration_structure, Buffer* indices, Buffer* vertices, Buffer* normals, Buffer* texcoords, Buffer* tangents, Buffer* mesh_data_offsets, Buffer* mesh_offset_indices, std::vector<Image>* loaded_textures, Buffer* texture_indices, Buffer* material_parameters, Buffer* lights, Buffer* previous_camera_data);

    void initialize();
    void on_resize(VkExtent2D swapchain_extent, VkExtent2D render_extent);
    void process(VkCommandBuffer command_buffer, VkExtent2D swapchain_extent, VkExtent2D render_extent, Shaders::PushConstantsPacked &push_constants_packed) override;
    void free();
};