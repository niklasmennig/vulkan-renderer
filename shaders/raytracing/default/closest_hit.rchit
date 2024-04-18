#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_ray_query : enable
#extension GL_GOOGLE_include_directive : enable

#include "payload.glsl"
#include "../mesh_data.glsl"
#include "../texture_data.glsl"
#include "../material.glsl"
#include "../../common.glsl"
#include "../../random.glsl"
#include "../../push_constants.glsl"
#include "../bsdf.glsl"
#include "../environment.glsl"
#include "../lights.glsl"
#include "../restir.glsl"
#include "../mis.glsl"
#include "../output.glsl"

hitAttributeEXT vec2 barycentrics;

layout(location = 0) rayPayloadInEXT RayPayload payload;

layout(set = DESCRIPTOR_SET_FRAMEWORK, binding = DESCRIPTOR_BINDING_ACCELERATION_STRUCTURE) uniform accelerationStructureEXT as;

void main() {
    PushConstants constants = get_push_constants();
    uint sample_count = constants.sample_count;

    uint instance = gl_InstanceID;
    uint primitive = gl_PrimitiveID;

    mat4x3 transform_world = gl_ObjectToWorldEXT;
    mat4x3 transform_object = gl_WorldToObjectEXT;

    vec3 position = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    vec3 normal = normalize(transform_world * vec4(get_vertex_normal(instance, primitive, barycentrics), 0.0));
    vec3 face_normal = normalize(transform_world * vec4(get_face_normal(instance, primitive), 0.0));
    vec2 uv = get_vertex_uv(instance, primitive, barycentrics);
    vec3 tangent = normalize(transform_world * vec4(get_vertex_tangent(instance, primitive, barycentrics), 0.0));


    vec3 bitangent = normalize(cross(normal, tangent));
    mat3 tbn = basis(normal);
    
    //normal mapping
    if (has_texture(instance, TEXTURE_OFFSET_NORMAL)) {
        vec3 normal_tex = sample_texture(instance, uv, TEXTURE_OFFSET_NORMAL).rbg;
        vec3 sampled_normal = (normal_tex - 0.5) * 2.0;
        normal = normalize(tbn * sampled_normal);
    }

    mat3 to_world_space = basis(normal);
    mat3 to_shading_space = transpose(to_world_space);

    // pointing away from surface
    vec3 ray_out = -normalize(to_shading_space * gl_WorldRayDirectionEXT);

    // material properties
    Material material = get_material(instance, uv);

    if (payload.depth == 1) {
        payload.primary_hit_position = position;
        payload.primary_hit_uv = uv;

        if (sample_count == 1) {
            write_output(OUTPUT_BUFFER_INSTANCE, payload.pixel_index, vec4(encode_uint(instance), 0.0)); 
            write_output(OUTPUT_BUFFER_INSTANCE_COLOR, payload.pixel_index, vec4(random_vec3(instance), 1.0));

            write_output(OUTPUT_BUFFER_ALBEDO, payload.pixel_index, vec4(material.base_color, 1.0)); 
            write_output(OUTPUT_BUFFER_NORMAL, payload.pixel_index, vec4(normal, 0.0)); 
            write_output(OUTPUT_BUFFER_ROUGHNESS, payload.pixel_index, vec4(material.roughness)); 
            write_output(OUTPUT_BUFFER_POSITION, payload.pixel_index, vec4(position, 1.0));
            write_output(OUTPUT_BUFFER_UV, payload.pixel_index, vec4(uv, 0.0, 0.0));
        }
    }


    uint bsdf_seed = random_uint(payload.seed);
    BSDFSample bsdf_sample = sample_bsdf(ray_out, material, bsdf_seed);

    // pointing away from surface
    vec3 ray_in = bsdf_sample.direction;

    payload.origin = position;
    payload.direction = (to_world_space * ray_in);
    
    // direct lighting
    if ((constants.flags & ENABLE_DIRECT_LIGHTING) == ENABLE_DIRECT_LIGHTING) {
        uint nee_seed = random_uint(payload.seed);
        LightSample light_sample = sample_direct_light(nee_seed, position);

        rayQueryEXT ray_query;
        rayQueryInitializeEXT(ray_query, as, gl_RayFlagsTerminateOnFirstHitEXT, 0xff, position, EPSILON, light_sample.direction, light_sample.distance - 2.0 * EPSILON);
        rayQueryProceedEXT(ray_query);

        if (rayQueryGetIntersectionTypeEXT(ray_query, true) == gl_RayQueryCommittedIntersectionNoneEXT) {
            // pointing away from surface
            vec3 ray_dir_local = ray_out;

            // pointing away from surface
            vec3 light_dir_local = normalize(to_shading_space * light_sample.direction);

            BSDFEvaluation bsdf_eval = eval_bsdf(ray_dir_local, light_dir_local, material);

            vec3 nee_contribution = max(vec3(0.0), bsdf_eval.color * payload.contribution * light_sample.weight * abs(light_dir_local.y));

            float mis = 1.0;
            // if ((constants.flags & ENABLE_INDIRECT_LIGHTING) == ENABLE_INDIRECT_LIGHTING) mis = 1.0 / (1.0 + bsdf_eval.pdf / light_sample.pdf);
            payload.color += mis * nee_contribution;
        }
    }
    
    payload.color += max(vec3(0.0), material.emission * payload.contribution.rgb);

    payload.contribution *= max(vec3(0.0), bsdf_sample.weight);
    payload.last_bsdf_pdf_inv = 1.0 / bsdf_sample.pdf;

    payload.depth += 1;

    if (payload.depth > 3) {
        float rr_probability = luminance(payload.contribution);
        if (random_float(payload.seed) > rr_probability) {
            payload.contribution /= max(0, rr_probability);
        } else {
            return;
        }
    }

    if (payload.depth <= constants.max_depth) {
        traceRayEXT(
            as,
            0,
            0xff,
            0,
            constants.sbt_stride,
            0,
            payload.origin,
            EPSILON,
            payload.direction,
            RAY_LEN_MAX,
            0
        );
    }
}