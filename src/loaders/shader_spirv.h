#pragma once

#include <vector>
#include <string>
#include <fstream>

namespace loaders {
    static std::vector<char> read_spirv(const std::string filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("unable to open spirv file at " + filename);
        }

        size_t file_size = (size_t)file.tellg();

        std::vector<char> buffer(file_size);

        file.seekg(0);
        file.read(buffer.data(), file_size);
        file.close();

        return buffer;
    }

    static VkShaderModule load_shader_module(VkDevice device, const std::string filename) {
        auto code = read_spirv(filename);

        VkShaderModuleCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        create_info.codeSize = code.size();
        create_info.pCode = reinterpret_cast<const uint32_t *>(code.data());
        create_info.flags = 0;
        create_info.pNext = nullptr;

        VkShaderModule shader_module;
        if (vkCreateShaderModule(device, &create_info, nullptr, &shader_module) != VK_SUCCESS)
        {
            throw std::runtime_error("error creating shader module");
        }

        return shader_module;
    }
}