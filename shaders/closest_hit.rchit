#version 460 core
#extension GL_EXT_ray_tracing : enable

~include "shaders/common.glsl"

~include "shaders/payload.glsl"

hitAttributeEXT vec3 barycentrics;

layout(location = 0) rayPayloadInEXT RayPayload payload;
layout(set = 0, binding = 1) uniform accelerationStructureEXT as;
layout(set = 0, binding = 3) readonly buffer IndexData {uint normal_indices[];} index_data;
layout(set = 0, binding = 4) readonly buffer MeshData {vec4 normals[];} mesh_data;

void main() {
    // indices into mesh data
    uint idx0 = gl_PrimitiveID * 3 + 0;
    uint idx1 = gl_PrimitiveID * 3 + 1;
    uint idx2 = gl_PrimitiveID * 3 + 2;

    vec3 normal0 = mesh_data.normals[index_data.normal_indices[idx0]].xyz;
    vec3 normal1 = mesh_data.normals[index_data.normal_indices[idx1]].xyz;
    vec3 normal2 = mesh_data.normals[index_data.normal_indices[idx2]].xyz;
    vec3 normal = normalize(normal0 * barycentrics.x + normal1 * barycentrics.y + normal2 * barycentrics.z);
    
    vec3 color = vec3(1,1,1);
    
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
    float reflection = 0.3;
    if (reflection > 0.0) {
        payload.next_origin = hit_position;
        payload.next_direction = reflect(gl_WorldRayDirectionEXT, normal);
        payload.next_reflection_factor = reflection;
    } else {
        payload.next_origin = vec3(0,0,0);
        payload.next_direction = vec3(0,0,0);
    }

}