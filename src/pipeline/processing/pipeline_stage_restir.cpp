#include "pipeline/processing/pipeline_stage_restir.h"

#include "pipeline/processing/pipeline_builder.h"
#include "pipeline/processing/compute_shader.h"

#include "pipeline/raytracing/pipeline_builder.h"
#include "shader_interface.h"

#include <iostream>

ProcessingPipelineStageRestir::ProcessingPipelineStageRestir(VkAccelerationStructureKHR acceleration_structure, Buffer* indices, Buffer* vertices, Buffer* normals, Buffer* texcoords, Buffer* tangents, Buffer* mesh_data_offsets, Buffer* mesh_offset_indices, std::vector<Image>* loaded_textures, Buffer* texture_indices, Buffer* material_parameters, Buffer* lights, Buffer* previous_camera_data) {
    this->acceleration_structure = acceleration_structure;
    this->indices = indices;
    this->vertices = vertices;
    this->normals = normals;
    this->texcoords = texcoords;
    this->tangents = tangents;
    this->mesh_data_offsets = mesh_data_offsets;
    this->mesh_offset_indices = mesh_offset_indices;
    this->loaded_textures = loaded_textures;
    this->texture_indices = texture_indices;
    this->material_parameters = material_parameters;
    this->lights = lights;
    this->previous_camera_data = previous_camera_data;
}

void ProcessingPipelineStageRestir::initialize() {
    compute_shader = builder->create_compute_shader("./shaders/processing/restir_spatial.comp");
    compute_shader->build();

    compute_shader->set_acceleration_structure(0, acceleration_structure);

    compute_shader->set_buffer(3, lights);

    compute_shader->set_buffer(4, indices);
    compute_shader->set_buffer(5, vertices);
    compute_shader->set_buffer(6, normals);
    compute_shader->set_buffer(7, texcoords);
    compute_shader->set_buffer(8, tangents);
    compute_shader->set_buffer(9, mesh_data_offsets);
    compute_shader->set_buffer(10, mesh_offset_indices);

    compute_shader->set_buffer(11, texture_indices);
    compute_shader->set_images(0, loaded_textures);

    compute_shader->set_buffer(12, material_parameters);
    compute_shader->set_buffer(13, previous_camera_data);
}

void ProcessingPipelineStageRestir::on_resize(VkExtent2D swapchain_extent, VkExtent2D render_extent) {
    // required AOVs
    compute_shader->set_buffer(1, &builder->rt_pipeline->get_output_buffer("Instance Indices").buffer, 0);
    compute_shader->set_buffer(1, &builder->rt_pipeline->get_output_buffer("Position").buffer, 1);
    compute_shader->set_buffer(1, &builder->rt_pipeline->get_output_buffer("Normals").buffer, 2);
    compute_shader->set_buffer(1, &builder->rt_pipeline->get_output_buffer("UV").buffer, 3);

    // restir buffers
    for (int i = 0; i < 2; i++) {
        restir_buffers[i].free();
        restir_buffers[i] = builder->create_buffer(sizeof(float) * 4 * swapchain_extent.width * swapchain_extent.height);
        compute_shader->set_buffer(2, &restir_buffers[i], i);
    }
}

void ProcessingPipelineStageRestir::process(VkCommandBuffer command_buffer, VkExtent2D swapchain_extent, VkExtent2D render_extent, Shaders::PushConstantsPacked &push_constants_packed) {
    Buffer* image_buffer = &builder->rt_pipeline->get_output_buffer("Result Image").buffer;
    compute_shader->set_buffer(0, image_buffer);
    compute_shader->set_buffer(13, previous_camera_data);
    
    compute_shader->dispatch(command_buffer, swapchain_extent, render_extent, push_constants_packed);
}

void ProcessingPipelineStageRestir::free() {
    for (int i = 0; i < 2; i++) {
        restir_buffers[i].free();
    }
}