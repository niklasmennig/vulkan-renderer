#pragma once

#include "core/vulkan.h"
#include <string>

// a single stage in the render pipeline
// this might be a ray generation shader, closest hit shader etc.
// can also be a post-processing step
class PipelineStage {

public:
    // needed for polymorphic usage of stages
    virtual ~PipelineStage() = default;

    // initialize data bindings if needed
    virtual void initialize() = 0;

    // return shader flag bit that this pipeline stage should occupy
    // can be either VK_SHADER_STAGE_RAYGEN_BIT_KHR, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, VK_SHADER_STAGE_MISS_BIT_KHR or VK_SHADER_STAGE_CALLABLE_BIT_KHR
    virtual VkShaderStageFlagBits get_shader_stage() = 0;

    // return path of text file that contains GLSL shader code for this pipeline stage
    virtual std::string get_shader_code_path() = 0;

    // return entry point of the stage's shader program
    // default: "main"
    virtual const char* get_entry_point() { return "main"; };
};