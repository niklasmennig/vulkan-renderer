#version 460 core
#extension GL_EXT_ray_tracing : enable

float epsilon = 0.00001f;
float ray_max = 10000.0f;
struct RayPayload
{
    vec3 next_origin;
    vec3 next_direction;
    vec3 direct_light;
    bool shadow_miss;
    float next_reflection_factor;
};
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
hitAttributeEXT vec2 barycentrics;

layout(location = 0) rayPayloadInEXT RayPayload payload;
layout(set = 0, binding = 1) uniform accelerationStructureEXT as;
layout(set = 1, binding = 6) uniform sampler2D tex;


void main() {
    // indices into mesh data
    vec3 normal = get_vertex_normal(barycentrics);
    vec2 uv = get_vertex_uv(barycentrics);
    
    vec3 color = texture(tex, uv).rgb;
    
    vec3 light_dir = normalize(vec3(1, -1, 0));
    vec3 hit_position = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    payload.shadow_miss = false;

    traceRayEXT(
        as,
        gl_RayFlagsOpaqueEXT | gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsSkipClosestHitShaderEXT,
        0xff,
        0,
        0,
        0,
        hit_position + normal * epsilon,
        0.0,
        -light_dir,
        ray_max,
        0
    );

    // direct lighting
    vec3 radiance = vec3(0,0,0);
    if (payload.shadow_miss) {
        float irradiance = max(dot(-light_dir, normal), 0.0);
        radiance += color * irradiance;
    }
    payload.direct_light = radiance;

    // indirect bounce
    float reflection = 0.6;
    if (reflection > 0.0) {
        payload.next_origin = hit_position;
        payload.next_direction = reflect(gl_WorldRayDirectionEXT, normal);
        payload.next_reflection_factor = reflection;
    } else {
        payload.next_origin = vec3(0,0,0);
        payload.next_direction = vec3(0,0,0);
    }

}