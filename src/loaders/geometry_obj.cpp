#include "geometry_obj.h"
#include <glm/vec4.hpp>

#include <iostream>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

LoadedMeshData loaders::load_obj(const std::string path) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;
    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, path.c_str(), nullptr, true)) {
        std::cerr << "OBJ LOADING ERROR: " << err << std::endl;
        return LoadedMeshData{};
    }

    LoadedMeshData result;

    for (int vi = 0; vi < attrib.vertices.size(); vi += 3) {
        glm::vec4 pos = glm::vec4(attrib.vertices[vi + 0], attrib.vertices[vi + 1], attrib.vertices[vi + 2], 1.0f);
        result.vertices.push_back(pos);
    }

    for (int vn = 0; vn < attrib.normals.size(); vn += 3)  {
        glm::vec4 norm = glm::vec4(attrib.normals[vn + 0], attrib.normals[vn + 1], attrib.normals[vn + 2], 1.0f);
        result.normals.push_back(norm);
    }

    for (size_t shape_index = 0; shape_index < shapes.size(); shape_index++) {
        size_t index_offset = 0;
        for (size_t face_index = 0; face_index < shapes[shape_index].mesh.num_face_vertices.size(); face_index++) {
            size_t face_vertices = size_t(shapes[shape_index].mesh.num_face_vertices[face_index]);
            for (size_t vertex_index = 0; vertex_index < face_vertices; vertex_index++) {
                tinyobj::index_t idx = shapes[shape_index].mesh.indices[index_offset + vertex_index];

                result.vertex_indices.push_back((uint32_t)idx.vertex_index);
                result.normal_indices.push_back((uint32_t)idx.normal_index);
            }
            index_offset += face_vertices;
        }
    }

    std::cout << "loaded mesh with " << result.vertices.size() << " vertices" << " and " << result.vertex_indices.size() << " vertex indices" << std::endl;

    return result;
}