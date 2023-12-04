#include "pipeline/processing/pipeline_stage.h"

struct Image;
struct ComputeShader;

struct ProcessingPipelineStageSimple : ProcessingPipelineStage {
    Image* output_image;
    ComputeShader* compute_shader;

    void initialize(ProcessingPipelineBuilder* builder);
    void on_resize(ProcessingPipelineBuilder* builder, VkExtent2D image_extent);
    void process(VkCommandBuffer command_buffer);
};