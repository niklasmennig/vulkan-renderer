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
    // diffuse rgb, roughness a
    vec4 diffuse_roughness_factor;
    // emissive rgb, metallic a
    vec4 emissive_metallic_factor;
    // transmissive factor x, ior y
    vec4 transmissive_factor_ior;
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

    vec3 sampled_normal = sample_texture(instance, uv, TEXTURE_OFFSET_NORMAL).xyz;

    if (abs(length(sampled_normal)) > 1.0 - epsilon) {
        vec3 mapped_normal = (tbn * normalize(sampled_normal * 2.0 - 1.0));
        normal = normalize(mapped_normal);
    }

    vec4 base_color_tex = sample_texture(instance, uv, TEXTURE_OFFSET_DIFFUSE);
    vec3 base_color = parameters.diffuse_roughness_factor.rgb * base_color_tex.rgb;
    vec3 arm = sample_texture(instance, uv, TEXTURE_OFFSET_ROUGHNESS).rgb;
    float roughness = parameters.diffuse_roughness_factor.a * arm.y;
    float metallic = parameters.emissive_metallic_factor.a * arm.z;
    vec4 transmission_tex = sample_texture(instance, uv, TEXTURE_OFFSET_TRANSMISSIVE);
    float transmission = parameters.transmissive_factor_ior.x * (1.0 - transmission_tex.x);
    float ior = parameters.transmissive_factor_ior.y;

    float fresnel_reflect = 0.5;

    bool front_facing = dot(ray_out, normal) > 0;
    if (!front_facing) ior = 1.0 / ior;

    base_color = mix(base_color, vec3(1.0), 1.0 - base_color_tex.a);

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

    if (!in_shadow && transmission < epsilon) {
        payload.color += ggx(light_dir, ray_out, normal, base_color, metallic, fresnel_reflect, roughness, transmission, ior) * light_intensity * light_attenuation * max(0, dot(light_dir, normal));
    }

    // indirect light
    vec3 next_factor = vec3(0);
    vec3 r = sample_ggx(ray_out, normal, base_color, metallic, fresnel_reflect, roughness, transmission, ior, vec3(seed_random(payload.seed), seed_random(payload.seed), seed_random(payload.seed)), next_factor);

    vec3 new_direction = normalize(r);
    payload.contribution *= next_factor;

    // emission
    vec3 emission = parameters.emissive_metallic_factor.rgb * sample_texture(instance, uv, TEXTURE_OFFSET_EMISSIVE).rgb;
    payload.contribution += emission;

    if (payload.depth == 0) {
        payload.primary_hit_instance = instance;
        payload.primary_hit_albedo = base_color;
    }

    // russian roulette
    if (payload.depth > 3 && luminance(payload.contribution) < seed_random(payload.seed)) return;

    if (payload.depth < payload.max_depth) {
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