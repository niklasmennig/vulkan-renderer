#version 460 core
#extension GL_EXT_ray_tracing : enable

~include "shaders/common.glsl"
~include "shaders/payload.glsl"
~include "shaders/mesh_data.glsl"
~include "shaders/texture_data.glsl"

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