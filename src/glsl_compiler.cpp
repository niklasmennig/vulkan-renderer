#include "glsl_compiler.h"

#include "glslang/Public/ShaderLang.h"

void GLSLCompiler::initialize() {
    glslang::InitializeProcess();
    //glslang::GlslangToSpv()
}