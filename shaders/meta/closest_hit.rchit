#version 460 core
#extension GL_EXT_ray_tracing : enable

#define PI 3.1415926535897932384626433832795

float epsilon = 0.001f;
float ray_max = 1000.0f;


// taken from https://www.shadertoy.com/view/tlVczh
void basis(in vec3 n, out vec3 f, out vec3 r)
{
   //looks good but has this ugly branch
  if(n.z < -0.99995)
    {
        f = vec3(0 , -1, 0);
        r = vec3(-1, 0, 0);
    }
    else
    {
    	float a = 1./(1. + n.z);
    	float b = -n.x*n.y*a;
    	f = vec3(1. - n.x*n.x*a, b, -n.x);
    	r = vec3(b, 1. - n.y*n.y*a , -n.y);
    }

    f = normalize(f);
    r = normalize(r);
}struct RayPayload
{
    float hit_t;
    vec3 hit_position;
    vec3 hit_normal;
    vec2 hit_uv;
    uint hit_instance;
    uint hit_primitive;
};layout(set = 1, binding = 0) readonly buffer VertexData {vec4 data[];} vertices;
layout(set = 1, binding = 1) readonly buffer VertexIndexData {uint data[];} vertex_indices;
layout(set = 1, binding = 2) readonly buffer NormalData {vec4 data[];} normals;
layout(set = 1, binding = 3) readonly buffer NormalIndexData {uint data[];} normal_indices;
layout(set = 1, binding = 4) readonly buffer TexcoordData {vec2 data[];} texcoords;
layout(set = 1, binding = 5) readonly buffer TexcoordIndexData {uint data[];} texcoord_indices;
layout(set = 1, binding = 6) readonly buffer OffsetData {uint data[];} mesh_data_offsets;
layout(set = 1, binding = 7) readonly buffer OffsetIndexData {uint data[];} mesh_offset_indices;

vec3 get_vertex_position(uint instance, vec2 barycentric_coordinates) {
    uint idx0 = gl_PrimitiveID * 3 + 0;
    uint idx1 = gl_PrimitiveID * 3 + 1;
    uint idx2 = gl_PrimitiveID * 3 + 2;

    uint data_offset = mesh_data_offsets.data[mesh_offset_indices.data[instance] * 6 + 0];
    uint index_offset = mesh_data_offsets.data[mesh_offset_indices.data[instance] * 6 + 1];

    vec3 vert0 = vertices.data[data_offset + vertex_indices.data[index_offset + idx0]].xyz;
    vec3 vert1 = vertices.data[data_offset + vertex_indices.data[index_offset + idx1]].xyz;
    vec3 vert2 = vertices.data[data_offset + vertex_indices.data[index_offset + idx2]].xyz;
    vec3 vert = (vert1 * barycentric_coordinates.x + vert2 * barycentric_coordinates.y + vert0 * (1.0 - barycentric_coordinates.x - barycentric_coordinates.y));

    return vert;
}

vec3 get_vertex_normal(uint instance, vec2 barycentric_coordinates) {
    uint idx0 = gl_PrimitiveID * 3 + 0;
    uint idx1 = gl_PrimitiveID * 3 + 1;
    uint idx2 = gl_PrimitiveID * 3 + 2;

    uint data_offset = mesh_data_offsets.data[mesh_offset_indices.data[instance] * 6 + 2];
    uint index_offset = mesh_data_offsets.data[mesh_offset_indices.data[instance] * 6 + 3];

    vec3 norm0 = normals.data[data_offset + normal_indices.data[index_offset + idx0]].xyz;
    vec3 norm1 = normals.data[data_offset + normal_indices.data[index_offset + idx1]].xyz;
    vec3 norm2 = normals.data[data_offset + normal_indices.data[index_offset + idx2]].xyz;
    vec3 norm = (norm1 * barycentric_coordinates.x + norm2 * barycentric_coordinates.y + norm0 * (1.0 - barycentric_coordinates.x - barycentric_coordinates.y));

    return normalize(norm);
}

vec2 get_vertex_uv(uint instance, vec2 barycentric_coordinates) {
    uint idx0 = gl_PrimitiveID * 3 + 0;
    uint idx1 = gl_PrimitiveID * 3 + 1;
    uint idx2 = gl_PrimitiveID * 3 + 2;

    uint data_offset = mesh_data_offsets.data[mesh_offset_indices.data[instance] * 6 + 4];
    uint index_offset = mesh_data_offsets.data[mesh_offset_indices.data[instance] * 6 + 5];

    vec2 uv0 = texcoords.data[data_offset + texcoord_indices.data[index_offset + idx0]];
    vec2 uv1 = texcoords.data[data_offset + texcoord_indices.data[index_offset + idx1]];
    vec2 uv2 = texcoords.data[data_offset + texcoord_indices.data[index_offset + idx2]];
    vec2 uv = (uv1 * barycentric_coordinates.x + uv2 * barycentric_coordinates.y + uv0 * (1.0 - barycentric_coordinates.x - barycentric_coordinates.y));

    return uv;
}
hitAttributeEXT vec2 barycentrics;

layout(location = 0) rayPayloadInEXT RayPayload payload;


void main() {
    // indices into mesh data
    vec3 normal = get_vertex_normal(gl_InstanceID, barycentrics);
    vec2 uv = get_vertex_uv(gl_InstanceID, barycentrics);
    vec3 hit_position = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;

    payload.hit_t = gl_HitTEXT;
    payload.hit_position = hit_position;
    payload.hit_normal = normal;
    payload.hit_uv = uv;
    payload.hit_instance = gl_InstanceID;
    payload.hit_primitive = gl_PrimitiveID;
}