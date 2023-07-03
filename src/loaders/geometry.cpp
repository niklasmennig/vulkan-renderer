#include "geometry.h"

#include "../mikktspace/mikktspace.h"

int get_num_faces(const SMikkTSpaceContext* ctx) {
    LoadedMeshData* mesh_data = (LoadedMeshData*)ctx->m_pUserData;

    return mesh_data->indices.size() / 3;
}

int get_num_vertices_of_face(const SMikkTSpaceContext* ctx, int face) {
    // only support triangles
    return 3;
}

void get_position(const SMikkTSpaceContext* ctx, float* out, int face, int vert) {
    LoadedMeshData* mesh_data = (LoadedMeshData*)ctx->m_pUserData;

    int idx = face * 3 + vert;
    int vertex_index = mesh_data->indices[idx];

    glm::vec4 vertex = mesh_data->vertices[vertex_index];

    out[0] = vertex.x;
    out[1] = vertex.y;
    out[2] = vertex.z;
}

void get_normal(const SMikkTSpaceContext* ctx, float* out, int face, int vert) {
    LoadedMeshData* mesh_data = (LoadedMeshData*)ctx->m_pUserData;

    int idx = face * 3 + vert;
    int normal_index = mesh_data->indices[idx];

    glm::vec4 normal = mesh_data->normals[normal_index];

    out[0] = normal.x;
    out[1] = normal.y;
    out[2] = normal.z;
}

void get_uv(const SMikkTSpaceContext* ctx, float* out, int face, int vert) {
    LoadedMeshData* mesh_data = (LoadedMeshData*)ctx->m_pUserData;

    int idx = face * 3 + vert;
    int uv_index = mesh_data->indices[idx];

    glm::vec2 uv = mesh_data->texcoords[uv_index];

    out[0] = uv.x;
    out[1] = uv.y;
}

void set_tangent(const SMikkTSpaceContext* ctx, const float* in, float f_sign, int face, int vert) {
    LoadedMeshData* mesh_data = (LoadedMeshData*)ctx->m_pUserData;

    int idx = face * 3 + vert;
    int tangent_idx = mesh_data->indices[idx];

    glm::vec4 tangent;
    tangent.x = in[0];
    tangent.y = in[1];
    tangent.z = in[2];
    tangent.w = f_sign;

    mesh_data->tangents[tangent_idx] = tangent;
}

void LoadedMeshData::calculate_tangents() {
    SMikkTSpaceInterface interface{};
    SMikkTSpaceContext ctx{};

    interface.m_getNumFaces = get_num_faces;
    interface.m_getNumVerticesOfFace = get_num_vertices_of_face;
    interface.m_getPosition = get_position;
    interface.m_getNormal = get_normal;
    interface.m_getTexCoord = get_uv;
    interface.m_setTSpaceBasic = set_tangent;

    ctx.m_pInterface = &interface;
    ctx.m_pUserData = this;

    tangents.resize(vertices.size());

    genTangSpaceDefault(&ctx);
}