#pragma once

#include "core/vulkan.h"
#include <string>

class PipelineStage {

public:
    virtual ~PipelineStage() = default;

    virtual void initialize() = 0;
    virtual VkShaderStageFlagBits get_shader_stage() = 0;
    virtual std::string get_shader_code_path() = 0;
    virtual const char* get_entry_point() = 0;
};