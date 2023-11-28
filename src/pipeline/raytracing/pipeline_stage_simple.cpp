#include "pipeline_stage_simple.h"

void RaytracingPipelineStageSimple::initialize() {}

VkShaderStageFlagBits RaytracingPipelineStageSimple::get_shader_stage() {
    return shader_stage;
}

std::string RaytracingPipelineStageSimple::get_shader_code_path() {
    return shader_code_path;
}

RaytracingPipelineStageSimple::RaytracingPipelineStageSimple(VkShaderStageFlagBits shader_stage, std::string code_path) {
    this->shader_stage = shader_stage;
    this->shader_code_path = code_path;
}