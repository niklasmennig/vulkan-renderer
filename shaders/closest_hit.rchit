#version 460 core
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_ray_query : enable
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : enable

#include "common.glsl"
#include "payload.glsl"
#include "mesh_data.glsl"
#include "texture_data.glsl"
#include "random.glsl"
#include "sampling.glsl"
#include "ggx.glsl"

hitAttributeEXT vec2 barycentrics;

layout(location = 0) rayPayloadInEXT RayPayload payload;
layout(set = 0, binding = 0) uniform accelerationStructureEXT as;

struct MaterialParameters {
    // diffuse rgba
    vec4 diffuse_factor;
    // emissive rgb, metallic a
    vec4 emissive_metallic_factor;
};
layout(std430, set = 1, binding = 8) readonly buffer MaterialParameterData {MaterialParameters[] data;} material_parameters;

MaterialParameters get_material_parameters(uint instance) {
    return material_parameters.data[instance];
}

void main() {
    uint instance = gl_InstanceID; 

    vec3 position = get_vertex_position(instance, barycentrics);
    vec3 normal = get_vertex_normal(instance, barycentrics);
    vec2 uv = get_vertex_uv(instance, barycentrics);

    vec3 ray_out = normalize(-gl_WorldRayDirectionEXT);
    vec3 new_origin = position;

    MaterialParameters parameters = get_material_parameters(instance);

    //normal mapping
    vec3 tangent, bitangent;
    calculate_tangents(instance, barycentrics, tangent, bitangent);
    mat3 tbn = mat3(tangent, bitangent, normal);

    vec3 sampled_normal = sample_texture(instance, uv, TEXTURE_OFFSET_NORMAL);

    if (abs(length(sampled_normal)) > 1.0 - epsilon) {
        vec3 mapped_normal = (tbn * normalize(sampled_normal * 2.0 - 1.0));
        normal = normalize(mapped_normal);
    }

    vec3 base_color = parameters.diffuse_factor.rgb * sample_texture(instance, uv, TEXTURE_OFFSET_DIFFUSE);
    vec3 arm = sample_texture(instance, uv, TEXTURE_OFFSET_ROUGHNESS);
    float roughness = arm.y;
    float metallic = parameters.emissive_metallic_factor.a * arm.z;

    float fresnel_reflect = 0.5;

    // direct light
    vec3 light_position = vec3(3,20,100);
    vec3 light_intensity = vec3(0.0);
    vec3 light_dir = light_position - new_origin;
    float light_dist = length(light_dir);
    light_dir /= light_dist;
    float light_attenuation = 1.0;

    rayQueryEXT ray_query;
    rayQueryInitializeEXT(ray_query, as, gl_RayFlagsOpaqueEXT | gl_RayFlagsTerminateOnFirstHitEXT, 0xFF, new_origin, epsilon, light_dir, light_dist - 2 * epsilon);
    while(rayQueryProceedEXT(ray_query)) {};
    bool in_shadow = (rayQueryGetIntersectionTypeEXT(ray_query, true) == gl_RayQueryCommittedIntersectionTriangleEXT);

    if (!in_shadow) {
        payload.color += ggx(light_dir, ray_out, normal, base_color, metallic, fresnel_reflect, roughness) * light_intensity * light_attenuation * max(0, dot(light_dir, normal));
    }

    // indirect light
    vec3 next_factor = vec3(0);
    vec3 r = sample_ggx(ray_out, normal, base_color, metallic, fresnel_reflect, roughness, vec3(seed_random(payload.seed), seed_random(payload.seed), seed_random(payload.seed)), next_factor);

    vec3 new_direction = normalize(r);
    payload.contribution *= next_factor;

    // emission
    vec3 emission = parameters.emissive_metallic_factor.rgb * sample_texture(instance, uv, TEXTURE_OFFSET_EMISSIVE);
    payload.contribution += emission;

    if (payload.depth == 0) {
        payload.primary_hit_instance = instance;
        payload.primary_hit_albedo = base_color;
    }

    if (payload.depth < max_depth && dot(normal, ray_out) > 0) {
        payload.depth += 1;
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