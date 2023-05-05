layout(set = 1, binding = 0) readonly buffer VertexIndexData {uint data[];} vertex_indices;
layout(set = 1, binding = 1) readonly buffer VertexData {vec4 data[];} vertices;
layout(set = 1, binding = 2) readonly buffer NormalIndexData {uint data[];} normal_indices;
layout(set = 1, binding = 3) readonly buffer NormalData {vec4 data[];} normals;
layout(set = 1, binding = 4) readonly buffer TexcoordIndexData {uint data[];} texcoord_indices;
layout(set = 1, binding = 5) readonly buffer TexcoordData {vec2 data[];} texcoords;

vec3 get_vertex_position(vec2 barycentric_coordinates) {
    uint idx0 = gl_PrimitiveID * 3 + 0;
    uint idx1 = gl_PrimitiveID * 3 + 1;
    uint idx2 = gl_PrimitiveID * 3 + 2;

    vec3 vert0 = vertices.data[vertex_indices.data[idx0]].xyz;
    vec3 vert1 = vertices.data[vertex_indices.data[idx1]].xyz;
    vec3 vert2 = vertices.data[vertex_indices.data[idx2]].xyz;
    vec3 vert = (vert1 * barycentric_coordinates.x + vert2 * barycentric_coordinates.y + vert0 * (1.0 - barycentric_coordinates.x - barycentric_coordinates.y));

    return vert;
}

vec3 get_vertex_normal(vec2 barycentric_coordinates) {
    uint idx0 = gl_PrimitiveID * 3 + 0;
    uint idx1 = gl_PrimitiveID * 3 + 1;
    uint idx2 = gl_PrimitiveID * 3 + 2;

    vec3 norm0 = normals.data[normal_indices.data[idx0]].xyz;
    vec3 norm1 = normals.data[normal_indices.data[idx1]].xyz;
    vec3 norm2 = normals.data[normal_indices.data[idx2]].xyz;
    vec3 norm = (norm1 * barycentric_coordinates.x + norm2 * barycentric_coordinates.y + norm0 * (1.0 - barycentric_coordinates.x - barycentric_coordinates.y));

    return norm;
}

vec2 get_vertex_uv(vec2 barycentric_coordinates) {
    uint idx0 = gl_PrimitiveID * 3 + 0;
    uint idx1 = gl_PrimitiveID * 3 + 1;
    uint idx2 = gl_PrimitiveID * 3 + 2;

    vec2 uv0 = texcoords.data[texcoord_indices.data[idx0]];
    vec2 uv1 = texcoords.data[texcoord_indices.data[idx1]];
    vec2 uv2 = texcoords.data[texcoord_indices.data[idx2]];
    vec2 uv = (uv1 * barycentric_coordinates.x + uv2 * barycentric_coordinates.y + uv0 * (1.0 - barycentric_coordinates.x - barycentric_coordinates.y));

    return uv;
}