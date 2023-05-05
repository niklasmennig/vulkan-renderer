#version 460 core
#extension GL_EXT_ray_tracing : enable

~include "shaders/common.glsl"

~include "shaders/payload.glsl"

~include "shaders/mesh_data.glsl"

hitAttributeEXT vec2 barycentrics;

layout(location = 0) rayPayloadInEXT RayPayload payload;
layout(set = 0, binding = 1) uniform accelerationStructureEXT as;
layout(set = 1, binding = 6) uniform sampler2D tex;


void main() {
    // indices into mesh data
    vec3 normal = get_vertex_normal(barycentrics);
    vec2 uv = get_vertex_uv(barycentrics);
    
    vec3 color = texture(tex, uv).rgb;
    
    vec3 light_dir = normalize(vec3(1, -1, 0));
    vec3 hit_position = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    payload.shadow_miss = false;

    traceRayEXT(
        as,
        gl_RayFlagsOpaqueEXT | gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsSkipClosestHitShaderEXT,
        0xff,
        0,
        0,
        0,
        hit_position + normal * epsilon,
        0.0,
        -light_dir,
        ray_max,
        0
    );

    // direct lighting
    vec3 radiance = vec3(0,0,0);
    if (payload.shadow_miss) {
        float irradiance = max(dot(-light_dir, normal), 0.0);
        radiance += color * irradiance;
    }
    payload.direct_light = radiance;

    // indirect bounce
    float reflection = 0.6;
    if (reflection > 0.0) {
        payload.next_origin = hit_position;
        payload.next_direction = reflect(gl_WorldRayDirectionEXT, normal);
        payload.next_reflection_factor = reflection;
    } else {
        payload.next_origin = vec3(0,0,0);
        payload.next_direction = vec3(0,0,0);
    }

}