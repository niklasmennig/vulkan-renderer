layout(set = 1, binding = 0) readonly buffer IndexData {uint data[];} indices;
layout(set = 1, binding = 1) readonly buffer VertexData {vec4 data[];} vertices;
layout(set = 1, binding = 2) readonly buffer NormalData {vec4 data[];} normals;
layout(set = 1, binding = 3) readonly buffer TexcoordData {vec2 data[];} texcoords;
layout(set = 1, binding = 4) readonly buffer OffsetData {uint data[];} mesh_data_offsets;
layout(set = 1, binding = 5) readonly buffer OffsetIndexData {uint data[];} mesh_offset_indices;

#define OFFSET_ENTRIES 4

vec3 get_vertex_position(uint instance, vec2 barycentric_coordinates) {
    uint idx0 = gl_PrimitiveID * 3 + 0;
    uint idx1 = gl_PrimitiveID * 3 + 1;
    uint idx2 = gl_PrimitiveID * 3 + 2;

    uint index_offset = mesh_data_offsets.data[mesh_offset_indices.data[instance] * OFFSET_ENTRIES + 0];
    uint data_offset = mesh_data_offsets.data[mesh_offset_indices.data[instance] * OFFSET_ENTRIES + 1];

    vec3 vert0 = vertices.data[data_offset + indices.data[index_offset + idx0]].xyz;
    vec3 vert1 = vertices.data[data_offset + indices.data[index_offset + idx1]].xyz;
    vec3 vert2 = vertices.data[data_offset + indices.data[index_offset + idx2]].xyz;
    vec3 vert = (vert0 * (1.0 - barycentric_coordinates.x - barycentric_coordinates.y) + vert1 * barycentric_coordinates.x + vert2 * barycentric_coordinates.y);

    return vec3(gl_ObjectToWorldEXT * vec4(vert, 1.0));
}

vec3 get_vertex_normal(uint instance, vec2 barycentric_coordinates) {
    uint idx0 = gl_PrimitiveID * 3 + 0;
    uint idx1 = gl_PrimitiveID * 3 + 1;
    uint idx2 = gl_PrimitiveID * 3 + 2;

    uint index_offset = mesh_data_offsets.data[mesh_offset_indices.data[instance] * OFFSET_ENTRIES + 0];
    uint data_offset = mesh_data_offsets.data[mesh_offset_indices.data[instance] * OFFSET_ENTRIES + 2];

    vec3 norm0 = normals.data[data_offset + indices.data[index_offset + idx0]].xyz;
    vec3 norm1 = normals.data[data_offset + indices.data[index_offset + idx1]].xyz;
    vec3 norm2 = normals.data[data_offset + indices.data[index_offset + idx2]].xyz;
    vec3 norm = (norm0 * (1.0 - barycentric_coordinates.x - barycentric_coordinates.y) + norm1 * barycentric_coordinates.x + norm2 * barycentric_coordinates.y);

    return normalize(vec3(gl_ObjectToWorldEXT * vec4(norm, 0.0)));
}

vec2 get_vertex_uv(uint instance, vec2 barycentric_coordinates) {
    uint idx0 = gl_PrimitiveID * 3 + 0;
    uint idx1 = gl_PrimitiveID * 3 + 1;
    uint idx2 = gl_PrimitiveID * 3 + 2;

    uint index_offset = mesh_data_offsets.data[mesh_offset_indices.data[instance] * OFFSET_ENTRIES + 0];
    uint data_offset = mesh_data_offsets.data[mesh_offset_indices.data[instance] * OFFSET_ENTRIES + 3];

    vec2 uv0 = texcoords.data[data_offset + indices.data[index_offset + idx0]];
    vec2 uv1 = texcoords.data[data_offset + indices.data[index_offset + idx1]];
    vec2 uv2 = texcoords.data[data_offset + indices.data[index_offset + idx2]];
    vec2 uv = (uv0 * (1.0 - barycentric_coordinates.x - barycentric_coordinates.y) + uv1 * barycentric_coordinates.x + uv2 * barycentric_coordinates.y);

    return uv;
}

void calculate_tangents(uint instance, vec2 barycentric_coordinates, out vec3 tu, out vec3 tv) {
    uint idx0 = gl_PrimitiveID * 3 + 0;
    uint idx1 = gl_PrimitiveID * 3 + 1;
    uint idx2 = gl_PrimitiveID * 3 + 2;

    uint index_offset = mesh_data_offsets.data[mesh_offset_indices.data[instance] * OFFSET_ENTRIES + 0];
    uint vertex_data_offset = mesh_data_offsets.data[mesh_offset_indices.data[instance] * OFFSET_ENTRIES + 1];
    uint uv_data_offset = mesh_data_offsets.data[mesh_offset_indices.data[instance] * OFFSET_ENTRIES + 3];

    vec3 vert0 = vertices.data[vertex_data_offset + indices.data[index_offset + idx0]].xyz;
    vec3 vert1 = vertices.data[vertex_data_offset + indices.data[index_offset + idx1]].xyz;
    vec3 vert2 = vertices.data[vertex_data_offset + indices.data[index_offset + idx2]].xyz;

    vec2 uv0 = texcoords.data[uv_data_offset + indices.data[index_offset + idx0]];
    vec2 uv1 = texcoords.data[uv_data_offset + indices.data[index_offset + idx1]];
    vec2 uv2 = texcoords.data[uv_data_offset + indices.data[index_offset + idx2]];

    vec2 tc = uv0;
    mat2 t = mat2(
        uv1 - tc,
        uv2 - tc
    );

    vec3 pc = vert0;
    mat2x3 p = mat2x3(
        vert1 - pc,
        vert2 - pc
    );

    tu = ((gl_ObjectToWorldEXT * vec4(p * vec2(t[1][1], -t[0][1]), 0)).xyz);
    tv = ((gl_ObjectToWorldEXT * vec4(p * vec2(-t[1][0], t[0][0]), 0)).xyz);
}