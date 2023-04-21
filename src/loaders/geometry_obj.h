#include <vector>
#include <string>

struct LoadedMeshData
{
    std::vector<float> vertices;
    std::vector<uint32_t> indices;
};

namespace loaders {
    LoadedMeshData load_obj(const std::string path); 
}