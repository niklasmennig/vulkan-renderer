#version 460 core
#extension GL_EXT_ray_tracing : enable

precision highp float;

~include "shaders/common.glsl"
~include "shaders/mesh_data.glsl"
~include "shaders/texture_data.glsl"
~include "shaders/random.glsl"
~include "shaders/payload.glsl"

hitAttributeEXT vec2 barycentrics;

layout(set = 0, binding = 1) uniform accelerationStructureEXT as;


layout(location = 0) rayPayloadInEXT RayPayload payload;
layout(location = 0) callableDataEXT float material_payload;

void main() {

    if (payload.depth > max_bounces) {
        payload.contribution = vec3(1.0);
        return;
    }

    // indices into mesh data
    vec3 normal = get_vertex_normal(gl_InstanceID, barycentrics);
    vec2 uv = get_vertex_uv(gl_InstanceID, barycentrics);
    vec3 hit_position = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;

    payload.depth += 1;

    float cosine_term = max(0, dot(gl_WorldRayDirectionEXT, -normal));

    // material_payload.instance = gl_InstanceID;
    // material_payload.position = hit_position;
    // material_payload.normal = normal;
    // material_payload.uv = uv;
    // material_payload.seed = payload.seed;

    //executeCallableEXT(0, 0);


    vec3 ray_origin = hit_position;
    // vec3 ray_direction = material_payload.reflection_direction;
    vec3 ray_direction = reflect(gl_WorldRayDirectionEXT, normal);

    traceRayEXT(
                as,
                gl_RayFlagsOpaqueEXT,
                0xff,
                0,
                0,
                0,
                ray_origin,
                epsilon,
                ray_direction,
                ray_max,
                0
            );

    vec3 irradiance = payload.contribution * max(0, dot(ray_direction, normal));

    // material_payload.emission = vec3(0.2);
    // material_payload.surface_color = vec3(0,1,0);

    // vec3 radiance = material_payload.emission + (material_payload.surface_color * irradiance * cosine_term);
    vec3 radiance = vec3(1.0);

    payload.contribution = radiance;
}