#ifndef MESH_DATA_GLSL
#define MESH_DATA_GLSL

#include "interface.glsl"

layout(set = DESCRIPTOR_SET_OBJECTS, binding = DESCRIPTOR_BINDING_MESH_INDICES) readonly buffer IndexData {uint data[];} indices;
layout(set = DESCRIPTOR_SET_OBJECTS, binding = DESCRIPTOR_BINDING_MESH_VERTICES) readonly buffer VertexData {vec4 data[];} vertices;
layout(set = DESCRIPTOR_SET_OBJECTS, binding = DESCRIPTOR_BINDING_MESH_NORMALS) readonly buffer NormalData {vec4 data[];} normals;
layout(set = DESCRIPTOR_SET_OBJECTS, binding = DESCRIPTOR_BINDING_MESH_TEXCOORDS) readonly buffer TexcoordData {vec2 data[];} texcoords;
layout(set = DESCRIPTOR_SET_OBJECTS, binding = DESCRIPTOR_BINDING_MESH_TANGENTS) readonly buffer TangentData {vec4 data[];} tangents;
layout(set = DESCRIPTOR_SET_OBJECTS, binding = DESCRIPTOR_BINDING_MESH_DATA_OFFSETS) readonly buffer OffsetData {uint data[];} mesh_data_offsets;
layout(set = DESCRIPTOR_SET_OBJECTS, binding = DESCRIPTOR_BINDING_MESH_OFFSET_INDICES) readonly buffer OffsetIndexData {uint data[];} mesh_offset_indices;

#define MESH_DATA_OFFSET_ENTRIES 5

void get_vertices(uint instance, uint primitive, out vec3 v0, out vec3 v1, out vec3 v2) {
    uint idx0 = primitive * 3 + 0;
    uint idx1 = primitive * 3 + 1;
    uint idx2 = primitive * 3 + 2;

    uint index_offset = mesh_data_offsets.data[mesh_offset_indices.data[instance] * MESH_DATA_OFFSET_ENTRIES + 0];
    uint data_offset = mesh_data_offsets.data[mesh_offset_indices.data[instance] * MESH_DATA_OFFSET_ENTRIES + 1];

    v0 = vertices.data[data_offset + indices.data[index_offset + idx0]].xyz;
    v1 = vertices.data[data_offset + indices.data[index_offset + idx1]].xyz;
    v2 = vertices.data[data_offset + indices.data[index_offset + idx2]].xyz;
}

vec3 get_vertex_position(uint instance, uint primitive, vec2 barycentrics) {
    vec3 vert0, vert1, vert2;
    get_vertices(instance, primitive, vert0, vert1, vert2);
    
    vec3 vert = (vert0 * (1.0 - barycentrics.x - barycentrics.y) + vert1 * barycentrics.x + vert2 * barycentrics.y);

    return vert;
}

vec3 get_vertex_normal(uint instance, uint primitive, vec2 barycentrics) {
    uint idx0 = primitive * 3 + 0;
    uint idx1 = primitive * 3 + 1;
    uint idx2 = primitive * 3 + 2;

    uint index_offset = mesh_data_offsets.data[mesh_offset_indices.data[instance] * MESH_DATA_OFFSET_ENTRIES + 0];
    uint data_offset = mesh_data_offsets.data[mesh_offset_indices.data[instance] * MESH_DATA_OFFSET_ENTRIES + 2];

    vec3 norm0 = normals.data[data_offset + indices.data[index_offset + idx0]].xyz;
    vec3 norm1 = normals.data[data_offset + indices.data[index_offset + idx1]].xyz;
    vec3 norm2 = normals.data[data_offset + indices.data[index_offset + idx2]].xyz;
    vec3 norm = (norm0 * (1.0 - barycentrics.x - barycentrics.y) + norm1 * barycentrics.x + norm2 * barycentrics.y);

    return normalize(norm);
}

vec3 get_face_normal(uint instance, uint primitive) {
    vec3 v0, v1, v2;
    get_vertices(instance, primitive, v0, v1, v2);

    return normalize(cross(v1-v0, v2-v0));
}

vec2 get_vertex_uv(uint instance, uint primitive, vec2 barycentrics) {
    uint idx0 = primitive * 3 + 0;
    uint idx1 = primitive * 3 + 1;
    uint idx2 = primitive * 3 + 2;

    uint index_offset = mesh_data_offsets.data[mesh_offset_indices.data[instance] * MESH_DATA_OFFSET_ENTRIES + 0];
    uint data_offset = mesh_data_offsets.data[mesh_offset_indices.data[instance] * MESH_DATA_OFFSET_ENTRIES + 3];

    vec2 uv0 = texcoords.data[data_offset + indices.data[index_offset + idx0]];
    vec2 uv1 = texcoords.data[data_offset + indices.data[index_offset + idx1]];
    vec2 uv2 = texcoords.data[data_offset + indices.data[index_offset + idx2]];
    vec2 uv = (uv0 * (1.0 - barycentrics.x - barycentrics.y) + uv1 * barycentrics.x + uv2 * barycentrics.y);

    return uv;
}

vec3 get_vertex_tangent(uint instance, uint primitive, vec2 barycentrics) {
    uint idx0 = primitive * 3 + 0;
    uint idx1 = primitive * 3 + 1;
    uint idx2 = primitive * 3 + 2;

    uint index_offset = mesh_data_offsets.data[mesh_offset_indices.data[instance] * MESH_DATA_OFFSET_ENTRIES + 0];
    uint data_offset = mesh_data_offsets.data[mesh_offset_indices.data[instance] * MESH_DATA_OFFSET_ENTRIES + 4];

    vec3 tang0 = tangents.data[data_offset + indices.data[index_offset + idx0]].xyz;
    vec3 tang1 = tangents.data[data_offset + indices.data[index_offset + idx1]].xyz;
    vec3 tang2 = tangents.data[data_offset + indices.data[index_offset + idx2]].xyz;
    vec3 tang = (tang0 * (1.0 - barycentrics.x - barycentrics.y) + tang1 * barycentrics.x + tang2 * barycentrics.y);

    return normalize(tang);
}

#endif