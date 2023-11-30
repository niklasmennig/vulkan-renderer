#include "shader_compiler.h"

#include <filesystem>
#include <iostream>
#include <sstream>

std::filesystem::path compile_shader(std::string filepath) {
    std::filesystem::path shader_path(filepath);
    std::filesystem::path shader_out_path(filepath);
    shader_path.make_preferred();
    shader_out_path.concat(".spv");
    shader_out_path.make_preferred();
    std::stringstream compile_command;
    compile_command << GLSLC_EXE << " --target-env=vulkan1.3 -O -o " << std::filesystem::absolute(shader_out_path) << " " << std::filesystem::absolute(shader_path);
    std::cout << "COMPILING SHADER: " << compile_command.str() << std::endl;
    system(compile_command.str().c_str());
    return shader_out_path;
}