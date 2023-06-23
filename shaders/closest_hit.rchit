#version 460 core
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_ray_query : enable
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : enable

~include "shaders/common.glsl"
~include "shaders/payload.glsl"
~include "shaders/mesh_data.glsl"
~include "shaders/texture_data.glsl"
~include "shaders/random.glsl"
~include "shaders/sampling.glsl"
~include "shaders/ggx.glsl"

hitAttributeEXT vec2 barycentrics;

layout(location = 0) rayPayloadInEXT RayPayload payload;
layout(set = 0, binding = 1) uniform accelerationStructureEXT as;

void main() {
    uint instance = gl_InstanceID;

    vec3 position = get_vertex_position(instance, barycentrics);
    vec3 normal = get_vertex_normal(instance, barycentrics);
    vec2 uv = get_vertex_uv(instance, barycentrics);

    vec3 ray_out = normalize(-gl_WorldRayDirectionEXT);
    vec3 new_origin = position;


    //normal mapping
    mat3 tbn = basis(normal);
    vec3 sampled_normal = normalize(sample_texture(instance, uv, TEXTURE_OFFSET_NORMAL) * 2.0 - 1.0);
    //sampled_normal = vec3(0,0,1);
    vec3 mapped_normal = normalize(tbn * sampled_normal);
    //normal = mapped_normal;

    vec3 base_color = sample_texture(instance, uv, TEXTURE_OFFSET_DIFFUSE);
    vec3 arm = sample_texture(instance, uv, TEXTURE_OFFSET_ROUGHNESS);
    float roughness = arm.g;
    float metallic = 0.0;
   
    float fresnel_reflect = 0.5;

    // direct light
    vec3 light_position = vec3(1,1,1);
    vec3 light_intensity = vec3(30.0);
    vec3 light_dir = light_position - new_origin;
    float light_dist = length(light_dir);
    light_dir /= light_dist;
    float light_attenuation = 1.0 / (light_dist * light_dist);

    rayQueryEXT ray_query;
    rayQueryInitializeEXT(ray_query, as, gl_RayFlagsOpaqueEXT | gl_RayFlagsTerminateOnFirstHitEXT, 0xFF, new_origin + normal * epsilon, 0, light_dir, light_dist - epsilon);
    while(rayQueryProceedEXT(ray_query)) {};
    bool in_shadow = (rayQueryGetIntersectionTypeEXT(ray_query, true) == gl_RayQueryCommittedIntersectionTriangleEXT);

    if (!in_shadow) {
        payload.color += ggx(light_dir, ray_out, normal, base_color, metallic, fresnel_reflect, roughness) * light_intensity * light_attenuation * max(0, dot(light_dir, normal));
    }

    // indirect light
    vec3 next_factor = vec3(0);
    vec3 r = sample_ggx(ray_out, normal, base_color, metallic, fresnel_reflect, roughness, random_vec3(payload.seed), next_factor);

    vec3 new_direction = normalize(r);
    payload.contribution *= next_factor;

    
    if (payload.depth < max_depth) {
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