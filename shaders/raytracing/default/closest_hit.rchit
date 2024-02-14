#version 460
#extension GL_EXT_ray_tracing : enable

#include "payload.glsl"
#include "../mesh_data.glsl"
#include "../texture_data.glsl"
#include "../material.glsl"
#include "../common.glsl"
#include "../random.glsl"
#include "../ggx.glsl"
#include "../environment.glsl"
#include "../lights.glsl"
#include "../restir.glsl"
#include "../mis.glsl"

hitAttributeEXT vec2 barycentrics;

layout(location = 0) rayPayloadInEXT RayPayload payload;
layout(location = 1) rayPayloadEXT bool shadowray_occluded;

layout(set = DESCRIPTOR_SET_FRAMEWORK, binding = DESCRIPTOR_BINDING_ACCELERATION_STRUCTURE) uniform accelerationStructureEXT as;

layout(std430, set = DESCRIPTOR_SET_FRAMEWORK, binding = DESCRIPTOR_BINDING_LIGHTS) readonly buffer LightsData {Light[] lights;} lights_data;
layout(std430, push_constant) uniform PConstants {PushConstants constants;} push_constants;

layout(std430, set = DESCRIPTOR_SET_FRAMEWORK, binding = DESCRIPTOR_BINDING_RESTIR_RESERVOIRS) buffer ReSTIRReservoirBuffers {Reservoir reservoirs[];} restir_reservoirs[];

LightSample sample_direct_light(inout uint seed, vec3 position) {
    LightSample light_sample;
    if (random_float(payload.seed) <= 0.5) {
        light_sample = sample_environment(payload.seed, push_constants.constants.environment_cdf_dimensions);
    } else {
        uint light_idx = uint(floor(push_constants.constants.light_count * random_float(payload.seed)));
        Light light = lights_data.lights[light_idx];
        light_sample = sample_light(position, payload.seed, light);
    }

    return light_sample;
}

void main() {

    uint instance = gl_InstanceID;
    uint primitive = gl_PrimitiveID;

    mat4x3 transform_world = gl_ObjectToWorldEXT;
    mat4x3 transform_object = gl_WorldToObjectEXT;

    vec3 position = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    vec3 normal = normalize(transform_world * vec4(get_vertex_normal(instance, primitive, barycentrics), 0.0));
    vec3 face_normal = normalize(transform_world * vec4(get_face_normal(instance, primitive), 0.0));
    vec2 uv = get_vertex_uv(instance, primitive, barycentrics);
    vec3 tangent = normalize(transform_world * vec4(get_vertex_tangent(instance, primitive, barycentrics), 0.0));

    vec3 ray_direction = gl_ObjectRayDirectionEXT;

    vec3 ray_out = normalize(-ray_direction);
    vec3 new_origin = position;

    vec3 bitangent = normalize(cross(normal, tangent));
    mat3 tbn = basis(normal);
    
    //normal mapping
    vec3 normal_tex = sample_texture(instance, uv, TEXTURE_OFFSET_NORMAL).rbg;
    vec3 sampled_normal = (normal_tex - 0.5) * 2.0;
    normal = normalize(tbn * sampled_normal);
    
    // material properties
    Material material = get_material(instance, uv);

    if (payload.depth == 1) {
        payload.primary_hit_instance = instance;
        payload.primary_hit_position = position;
        payload.primary_hit_uv = uv;
        payload.primary_hit_albedo = material.base_color;
        payload.primary_hit_normal = normal;
    }


    bool front_facing = dot(face_normal, -gl_WorldRayDirectionEXT) > EPSILON;

    BSDFSample bsdf_sample = sample_ggx(ray_out, material.base_color, material.opacity, material.metallic, material.fresnel, material.roughness, material.transmission, material.ior, payload.seed, front_facing);

    payload.origin = position;
    payload.direction = normalize(transform_world * vec4(bsdf_sample.direction, 0.0));

    payload.contribution *= bsdf_sample.contribution / bsdf_sample.pdf;
    
    // direct lighting
    if (!bsdf_sample.specular && front_facing && (push_constants.constants.flags & ENABLE_DIRECT_LIGHTING) == ENABLE_DIRECT_LIGHTING) {
        uint nee_seed = payload.seed;
        LightSample light_sample = sample_direct_light(payload.seed, position);

        shadowray_occluded = true;
        traceRayEXT(
            as,
            gl_RayFlagsTerminateOnFirstHitEXT,
            0xff,
            1,
            push_constants.constants.sbt_stride,
            1,
            position,
            EPSILON,
            light_sample.direction,
            light_sample.distance - 2.0 * EPSILON,
            1
        );

        if (!shadowray_occluded) {
            vec3 light_dir_local = transform_object * vec4(-light_sample.direction, 0.0);

            vec3 bsdf_eval = eval_ggx(ray_out, light_dir_local, material.base_color, material.opacity, material.metallic, material.fresnel, material.roughness, material.transmission, material.ior);  
            float bsdf_pdf = pdf_ggx(ray_out, light_dir_local, material.base_color, material.opacity, material.metallic, material.fresnel, material.roughness, material.transmission, material.ior);

            vec3 nee_contribution = bsdf_eval * payload.contribution * light_sample.intensity * clamp(dot(light_sample.direction, normal), 0, 1);

            if (payload.depth == 1) {
                RestirSample restir_sample;
                restir_sample.light_direction = vec4(light_sample.direction, 0.0);
                restir_sample.light_intensity = vec4(light_sample.intensity, 0.0);
                update_reservoir(restir_reservoirs[payload.restir_index].reservoirs[payload.pixel_index], restir_sample, light_sample.pdf, payload.seed);
            } else {
                float mis = balance_heuristic(1.0f, light_sample.pdf, 1.0f, bsdf_pdf); 
                payload.color += mis * nee_contribution;
            }
        } else {
            if (payload.depth == 1) {
                restir_reservoirs[payload.restir_index].reservoirs[payload.pixel_index].y.light_intensity = vec4(0.0);
                restir_reservoirs[payload.restir_index].reservoirs[payload.pixel_index].sum_weights = 0;
            }
        }
    }
}