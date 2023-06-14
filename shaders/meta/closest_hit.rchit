#version 460 core
#extension GL_EXT_ray_tracing : enable


#define PI 3.1415926535897932384626433832795

float epsilon = 0.001f;
float ray_max = 1000.0f;

uint max_depth = 0;

// taken from https://www.shadertoy.com/view/tlVczh
void basis(in vec3 n, out vec3 f, out vec3 r)
{
   //looks good but has this ugly branch
  if(n.z < -0.99995)
    {
        f = vec3(0.0 , -1.0, 0.0);
        r = vec3(-1.0, 0.0, 0.0);
    }
    else
    {
    	float a = 1.0/(1.0 + n.z);
    	float b = -n.x*n.y*a;
    	f = vec3(1.0 - n.x*n.x*a, b, -n.x);
    	r = vec3(b, 1.0 - n.y*n.y*a , -n.y);
    }

    f = normalize(f);
    r = normalize(r);
}

// https://stackoverflow.com/questions/596216/formula-to-determine-perceived-brightness-of-rgb-color
float luminance(vec3 color) {
    return (0.299*color.r + 0.587*color.g + 0.114*color.b);
}
#line 4

struct RayPayload
{
    vec3 color;
    vec3 contribution;

    uint depth;
};


struct MaterialPayload
{
    // ray data
    uint seed;

    // input data
    uint instance;
    vec3 position;
    vec3 normal;
    vec2 uv;
    vec3 direction;

    // output data
    vec3 emission;
    vec3 surface_color;
    vec3 sample_direction;
    float sample_pdf;
};
#line 5

layout(set = 1, binding = 0) readonly buffer VertexData {vec4 data[];} vertices;
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
    vec3 vert = (vert0 * (1.0 - barycentric_coordinates.x - barycentric_coordinates.y) + vert1 * barycentric_coordinates.x + vert2 * barycentric_coordinates.y);

    return vec3(gl_ObjectToWorldEXT * vec4(vert, 1.0));
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
    vec3 norm = (norm0 * (1.0 - barycentric_coordinates.x - barycentric_coordinates.y) + norm1 * barycentric_coordinates.x + norm2 * barycentric_coordinates.y);

    return normalize(vec3(gl_ObjectToWorldEXT * vec4(norm, 0.0)));
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
    vec2 uv = (uv0 * (1.0 - barycentric_coordinates.x - barycentric_coordinates.y) + uv1 * barycentric_coordinates.x + uv2 * barycentric_coordinates.y);

    return uv;
}
#line 6

layout(set = 1, binding = 8) uniform sampler2D tex[16];
layout(set = 1, binding = 9) readonly buffer TextureIndexData {uint data[];} texture_indices;

vec3 sample_texture(uint instance, vec2 uv) {
    return texture(tex[texture_indices.data[instance]], uv).rgb;
}
#line 7

hitAttributeEXT vec2 barycentrics;

layout(location = 0) rayPayloadInEXT RayPayload payload;
layout(set = 0, binding = 1) uniform accelerationStructureEXT as;

void main() {
    vec3 position = get_vertex_position(gl_InstanceID, barycentrics);
    vec3 normal = get_vertex_normal(gl_InstanceID, barycentrics);
    vec2 uv = get_vertex_uv(gl_InstanceID, barycentrics);
    uint instance = gl_InstanceID;

    vec3 ray_out = gl_WorldRayDirectionEXT;

    payload.contribution = payload.contribution * 0.5;
    payload.color = payload.color + payload.contribution * sample_texture(instance, uv);
    payload.color = vec3(1.0) * sample_texture(instance, uv) * abs(dot(ray_out, normal));
    vec3 new_origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    vec3 new_direction = reflect(gl_WorldRayDirectionEXT, normal);

    if (payload.depth < max_depth) {
        payload.depth = payload.depth + 1;
        traceRayEXT(
                as,
                gl_RayFlagsOpaqueEXT,
                0xff,
                0,
                0,
                0,
                new_origin,
                epsilon,
                new_direction,
                ray_max,
                0
            );
    }

}