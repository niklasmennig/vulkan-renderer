#include "pipeline_stage.h"
#include <string>

// simple pipeline stage defined by shader stage and shader program code
class PipelineStageSimple : public PipelineStage  {
    // shader stage this pipeline stage occupies
    VkShaderStageFlagBits shader_stage;
    // path to shader program code
    std::string shader_code_path;
public:
    VkShaderStageFlagBits get_shader_stage();
    std::string get_shader_code_path();
    void initialize();
    PipelineStageSimple(VkShaderStageFlagBits shader_stage, std::string code_path);
};