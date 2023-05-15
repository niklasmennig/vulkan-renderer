#version 460 core
#extension GL_EXT_ray_tracing : enable

precision highp float;

~include "shaders/common.glsl"
~include "shaders/mesh_data.glsl"
~include "shaders/texture_data.glsl"

hitAttributeEXT vec2 barycentrics;

layout(set = 0, binding = 1) uniform accelerationStructureEXT as;

layout(push_constant) uniform PushConstants {
    float time;
    int clear_accumulated;
} push_constants;

~include "shaders/random.glsl"
~include "shaders/payload.glsl"
layout(location = 0) rayPayloadInEXT RayPayload payload;

vec3 cosine_weighted_sample_hemisphere(float u1, float u2)
{
    float theta = 0.5 * acos(-2.0 * u1 + 1.0);
    float phi = u2 * 2.0 * PI;

    float x = cos(phi) * sin(theta);
    float y = sin(phi) * sin(theta);
    float z = cos(theta);

    return vec3(x, y, z);
}

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

    vec3 emission = vec3(0.0);
    if (gl_InstanceID == 0) emission = vec3(1.0);

    float cosine_term = max(0, dot(gl_WorldRayDirectionEXT, -normal));

    vec3 ray_origin = hit_position;
    //vec3 ray_direction = reflect(gl_WorldRayDirectionEXT, -normal);
    vec3 t, bt;
    basis(normal, t, bt);

    vec3 sample_dir = cosine_weighted_sample_hemisphere(seed_random(payload.seed), seed_random(payload.seed));
    vec3 ray_direction = normalize(normal * sample_dir.z + t * sample_dir.x + bt * sample_dir.y);

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

    vec3 irradiance = payload.contribution;
    vec3 color = sample_texture(gl_InstanceID, uv);
    //vec3 color = vec3(1.0, 0.0, 0.0);

    vec3 radiance = emission + color * irradiance;

    payload.contribution = radiance;
}