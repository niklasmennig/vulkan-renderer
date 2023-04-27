#version 460 core
#extension GL_EXT_ray_tracing : enable

~include "shaders/payload.glsl"

layout(location = 0) rayPayloadInEXT RayPayload payload;
layout(set = 0, binding = 1) uniform accelerationStructureEXT as;
layout(set = 0, binding = 3) readonly buffer IndexData {uint normal_indices[];} index_data;
layout(set = 0, binding = 4) readonly buffer MeshData {vec4 normals[];} mesh_data;

void main() {
    payload.distance = gl_HitTEXT;

    uint normal_index = index_data.normal_indices[gl_PrimitiveID * 3];
    vec3 normal = mesh_data.normals[normal_index].xyz;
    
    vec3 ray_dir = gl_ObjectRayDirectionEXT;
    float cos_term = clamp(dot(normalize(-ray_dir), normalize(normal)), 0.0, 1.0);

    vec3 color = vec3(1,0,0);
    
    vec3 light_dir = vec3(1, -1, 0);
    vec3 shadow_ray_origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT + normal * 0.01;
    traceRayEXT(
        as,
        gl_RayFlagsOpaqueEXT | gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsSkipClosestHitShaderEXT,
        0xff,
        0,
        0,
        0,
        shadow_ray_origin,
        0.0,
        -light_dir,
        100.0,
        0
    );

    vec4 light_contrib = vec4(1,1,1,1) * dot(-light_dir, normal);
    if (payload.distance < 9000) {
        light_contrib = vec4 (0,0,0,1);
    }

    payload.contribution = vec4(color,1.0);
    payload.contribution *= light_contrib;
}