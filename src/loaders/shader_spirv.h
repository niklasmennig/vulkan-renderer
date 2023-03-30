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
}