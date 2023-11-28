#include "pipeline_stage.h"
#include <string>

class PipelineStageSimple : public PipelineStage  {
    VkShaderStageFlagBits shader_stage;
    std::string shader_code_path;
public:
    VkShaderStageFlagBits get_shader_stage();
    std::string get_shader_code_path();
    void initialize();
    PipelineStageSimple(VkShaderStageFlagBits shader_stage, std::string code_path);
};