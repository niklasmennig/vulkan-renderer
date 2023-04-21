#include "geometry_obj.h"

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

    for (auto v : attrib.vertices) {
        result.vertices.push_back((float)v);
    }

    for (size_t shape_index = 0; shape_index < shapes.size(); shape_index++) {
        size_t index_offset = 0;
        for (size_t face_index = 0; face_index < shapes[shape_index].mesh.num_face_vertices.size(); face_index++) {
            size_t face_vertices = size_t(shapes[shape_index].mesh.num_face_vertices[face_index]);
            for (size_t vertex_index = 0; vertex_index < face_vertices; vertex_index++) {
                tinyobj::index_t idx = shapes[shape_index].mesh.indices[index_offset + vertex_index];

                result.indices.push_back((uint32_t)idx.vertex_index);
            }
            index_offset += face_vertices;
        }
    }

    std::cout << "Succesfully loaded " << result.vertices.size() << " vertices with " << result.indices.size() << " indices" << std::endl;

    return result;
}