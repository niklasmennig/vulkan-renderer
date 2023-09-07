#ifndef MESH_DATA_GLSL
#define MESH_DATA_GLSL

#include "payload.glsl"

layout(set = 1, binding = 0) readonly buffer IndexData {uint data[];} indices;
layout(set = 1, binding = 1) readonly buffer VertexData {vec4 data[];} vertices;
layout(set = 1, binding = 2) readonly buffer NormalData {vec4 data[];} normals;
layout(set = 1, binding = 3) readonly buffer TexcoordData {vec2 data[];} texcoords;
layout(set = 1, binding = 4) readonly buffer TangentData {vec4 data[];} tangents;
layout(set = 1, binding = 5) readonly buffer OffsetData {uint data[];} mesh_data_offsets;
layout(set = 1, binding = 6) readonly buffer OffsetIndexData {uint data[];} mesh_offset_indices;

#define OFFSET_ENTRIES 5

vec3 get_vertex_position(RayPayload payload) {
    uint idx0 = payload.hit_primitive * 3 + 0;
    uint idx1 = payload.hit_primitive * 3 + 1;
    uint idx2 = payload.hit_primitive * 3 + 2;

    uint index_offset = mesh_data_offsets.data[mesh_offset_indices.data[payload.hit_instance] * OFFSET_ENTRIES + 0];
    uint data_offset = mesh_data_offsets.data[mesh_offset_indices.data[payload.hit_instance] * OFFSET_ENTRIES + 1];

    vec3 vert0 = vertices.data[data_offset + indices.data[index_offset + idx0]].xyz;
    vec3 vert1 = vertices.data[data_offset + indices.data[index_offset + idx1]].xyz;
    vec3 vert2 = vertices.data[data_offset + indices.data[index_offset + idx2]].xyz;
    vec3 vert = (vert0 * (1.0 - payload.hit_barycentrics.x - payload.hit_barycentrics.y) + vert1 * payload.hit_barycentrics.x + vert2 * payload.hit_barycentrics.y);

    return vec3(payload.hit_transform * vec4(vert, 1.0));
}

vec3 get_vertex_normal(RayPayload payload) {
    uint idx0 = payload.hit_primitive * 3 + 0;
    uint idx1 = payload.hit_primitive * 3 + 1;
    uint idx2 = payload.hit_primitive * 3 + 2;

    uint index_offset = mesh_data_offsets.data[mesh_offset_indices.data[payload.hit_instance] * OFFSET_ENTRIES + 0];
    uint data_offset = mesh_data_offsets.data[mesh_offset_indices.data[payload.hit_instance] * OFFSET_ENTRIES + 2];

    vec3 norm0 = normals.data[data_offset + indices.data[index_offset + idx0]].xyz;
    vec3 norm1 = normals.data[data_offset + indices.data[index_offset + idx1]].xyz;
    vec3 norm2 = normals.data[data_offset + indices.data[index_offset + idx2]].xyz;
    vec3 norm = (norm0 * (1.0 - payload.hit_barycentrics.x - payload.hit_barycentrics.y) + norm1 * payload.hit_barycentrics.x + norm2 * payload.hit_barycentrics.y);

    return normalize(vec3(payload.hit_transform * vec4(norm, 0.0)));
}

vec2 get_vertex_uv(RayPayload payload) {
    uint idx0 = payload.hit_primitive * 3 + 0;
    uint idx1 = payload.hit_primitive * 3 + 1;
    uint idx2 = payload.hit_primitive * 3 + 2;

    uint index_offset = mesh_data_offsets.data[mesh_offset_indices.data[payload.hit_instance] * OFFSET_ENTRIES + 0];
    uint data_offset = mesh_data_offsets.data[mesh_offset_indices.data[payload.hit_instance] * OFFSET_ENTRIES + 3];

    vec2 uv0 = texcoords.data[data_offset + indices.data[index_offset + idx0]];
    vec2 uv1 = texcoords.data[data_offset + indices.data[index_offset + idx1]];
    vec2 uv2 = texcoords.data[data_offset + indices.data[index_offset + idx2]];
    vec2 uv = (uv0 * (1.0 - payload.hit_barycentrics.x - payload.hit_barycentrics.y) + uv1 * payload.hit_barycentrics.x + uv2 * payload.hit_barycentrics.y);

    return uv;
}

vec3 get_vertex_tangent(RayPayload payload) {
    uint idx0 = payload.hit_primitive * 3 + 0;
    uint idx1 = payload.hit_primitive * 3 + 1;
    uint idx2 = payload.hit_primitive * 3 + 2;

    uint index_offset = mesh_data_offsets.data[mesh_offset_indices.data[payload.hit_instance] * OFFSET_ENTRIES + 0];
    uint data_offset = mesh_data_offsets.data[mesh_offset_indices.data[payload.hit_instance] * OFFSET_ENTRIES + 4];

    vec3 tang0 = tangents.data[data_offset + indices.data[index_offset + idx0]].xyz;
    vec3 tang1 = tangents.data[data_offset + indices.data[index_offset + idx1]].xyz;
    vec3 tang2 = tangents.data[data_offset + indices.data[index_offset + idx2]].xyz;
    vec3 tang = (tang0 * (1.0 - payload.hit_barycentrics.x - payload.hit_barycentrics.y) + tang1 * payload.hit_barycentrics.x + tang2 * payload.hit_barycentrics.y);

    return normalize(vec3(payload.hit_transform * vec4(tang, 0.0)));
}

#endif