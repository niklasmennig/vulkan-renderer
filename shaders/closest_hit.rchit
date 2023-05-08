#version 460 core
#extension GL_EXT_ray_tracing : enable

~include "shaders/common.glsl"
~include "shaders/payload.glsl"
~include "shaders/mesh_data.glsl"

hitAttributeEXT vec2 barycentrics;

layout(location = 0) rayPayloadInEXT RayPayload payload;


void main() {
    // indices into mesh data
    vec3 normal = get_vertex_normal(barycentrics);
    vec2 uv = get_vertex_uv(barycentrics);
    vec3 hit_position = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;

    payload.hit_t = gl_HitTEXT;
    payload.hit_position = hit_position;
    payload.hit_normal = normal;
    payload.hit_uv = uv;
    payload.hit_instance = gl_InstanceID;
    payload.hit_primitive = gl_PrimitiveID;
}