#pragma once

struct Device;
struct ProcessingPipelineBuilder;

#include "core/vulkan.h"

struct ProcessingPipeline {
    Device* device;

    ProcessingPipelineBuilder* builder;

    void free();
};

struct ProcessingPipelineBuilder {
    Device* device;

    ProcessingPipeline build();

    void free();
};