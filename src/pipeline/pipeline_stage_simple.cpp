#include "pipeline_stage_simple.h"

void PipelineStageSimple::initialize() {}

VkShaderStageFlagBits PipelineStageSimple::get_shader_stage() {
    return shader_stage;
}

std::string PipelineStageSimple::get_shader_code_path() {
    return shader_code_path;
}

const char* PipelineStageSimple::get_entry_point() {
    return "main";
}

PipelineStageSimple::PipelineStageSimple(VkShaderStageFlagBits shader_stage, std::string code_path) {
    this->shader_stage = shader_stage;
    this->shader_code_path = code_path;
}