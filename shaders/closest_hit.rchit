#version 460 core
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_ray_query : enable
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : enable

#include "payload.glsl"
#include "mesh_data.glsl"
#include "texture_data.glsl"
#include "random.glsl"
#include "sampling.glsl"
#include "ggx.glsl"
#include "lambert.glsl"
#include "structs.glsl"

hitAttributeEXT vec2 barycentrics;

layout(location = 0) rayPayloadInEXT RayPayload payload;
layout(set = 0, binding = 0) uniform accelerationStructureEXT as;

struct MaterialParameters {
    // diffuse rgb, opacity a
    vec4 diffuse_opacity;
    // emissive rgb, emission strength a
    vec4 emissive_factor;
    // roughness x, metallic y, transmissive z, ior a
    vec4 roughness_metallic_transmissive_ior;
};
layout(std430, set = 1, binding = 9) readonly buffer MaterialParameterData {MaterialParameters[] data;} material_parameters;
layout(std430, set = 2, binding = 0) readonly buffer LightsData {Light[] lights;} lights_data;

layout( push_constant ) uniform PConstants {PushConstants constants;} push_constants;

MaterialParameters get_material_parameters(uint instance) {
    return material_parameters.data[instance];
}

void main() {
    uint instance = gl_InstanceID;

    vec3 position = get_vertex_position(instance, barycentrics);
    vec3 normal = get_vertex_normal(instance, barycentrics);
    vec2 uv = get_vertex_uv(instance, barycentrics);
    vec3 tangent = get_vertex_tangent(instance, barycentrics);

    vec3 ray_out = normalize(-gl_WorldRayDirectionEXT);
    vec3 new_origin = position;

    MaterialParameters parameters = get_material_parameters(instance);

    vec3 bitangent = normalize(cross(normal, tangent));
    mat3 tbn = mat3(tangent, bitangent, normal);
    
    //normal mapping
    vec3 normal_tex = sample_texture(instance, uv, TEXTURE_OFFSET_NORMAL).rgb;
    vec3 sampled_normal = (normal_tex - 0.5) * 2.0;

    // // COMPONENTS THAT SHOULD BE 0 ARE NOT MAPPING TO 0
    // // CHECK sampled_normal.x FOR EXAMPLE

    normal = normalize(tbn * sampled_normal);
    tbn[2] = normal;
    

    vec4 base_color_tex = sample_texture(instance, uv, TEXTURE_OFFSET_DIFFUSE);
    vec3 base_color = parameters.diffuse_opacity.rgb * base_color_tex.rgb;
    float opacity = parameters.diffuse_opacity.a * base_color_tex.a;
    vec3 arm = sample_texture(instance, uv, TEXTURE_OFFSET_ROUGHNESS).rgb;
    float roughness = parameters.roughness_metallic_transmissive_ior.x * arm.y;
    float metallic = parameters.roughness_metallic_transmissive_ior.y * arm.z;
    vec4 transmission_tex = sample_texture(instance, uv, TEXTURE_OFFSET_TRANSMISSIVE);
    float transmission = parameters.roughness_metallic_transmissive_ior.z * (1.0 - transmission_tex.x);
    float ior = parameters.roughness_metallic_transmissive_ior.a;

    float fresnel_reflect = 0.5;

    bool front_facing = dot(ray_out, normal) > 0;

    // direct light
    float rand = seed_random(payload.seed);
    Light selected_light = lights_data.lights[uint(floor(push_constants.constants.light_count * rand))];
    vec3 light_position = selected_light.position;
    vec3 light_intensity = selected_light.intensity;
    vec3 light_dir = light_position - new_origin;
    float light_dist = length(light_dir);
    light_dir /= light_dist;
    float light_attenuation = 1.0 / pow(light_dist, 2);

    rayQueryEXT ray_query;
    rayQueryInitializeEXT(ray_query, as, gl_RayFlagsOpaqueEXT | gl_RayFlagsTerminateOnFirstHitEXT, 0xFF, new_origin, epsilon, light_dir, light_dist - 2 * epsilon);
    while(rayQueryProceedEXT(ray_query)) {};
    bool in_shadow = (rayQueryGetIntersectionTypeEXT(ray_query, true) == gl_RayQueryCommittedIntersectionTriangleEXT);

    if (!in_shadow) {
        payload.color += ggx(light_dir, ray_out, tbn, base_color, opacity, metallic, fresnel_reflect, roughness, transmission, ior) * light_intensity * light_attenuation * max(0, dot(light_dir, normal));
    }


    // indirect light
    vec3 next_factor = vec3(0);
    vec4 random_values = vec4(seed_random(payload.seed), seed_random(payload.seed), seed_random(payload.seed), seed_random(payload.seed));

    // vec3 r = sample_ggx(ray_out, tbn, base_color, opacity, metallic, fresnel_reflect, roughness, transmission, ior, random_values, next_factor);
    vec3 r = sample_lambert(ray_out, tbn, base_color, random_values, next_factor);

    vec3 new_direction = normalize(r);
    payload.contribution *= next_factor;

    // emission
    vec3 emission = parameters.emissive_factor.rgb * sample_texture(instance, uv, TEXTURE_OFFSET_EMISSIVE).rgb;
    payload.contribution += emission * parameters.emissive_factor.a;

    if (payload.depth == 0) {
        payload.primary_hit_instance = instance;
        payload.primary_hit_albedo = base_color;
        payload.primary_hit_normal = normal;
        payload.primary_hit_roughness = vec3(roughness);
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