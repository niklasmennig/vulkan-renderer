#version 460 core
#extension GL_EXT_ray_tracing : enable

~include "shaders/payload.glsl"
~include "shaders/mesh_data.glsl"

hitAttributeEXT vec2 barycentrics;

layout(location = 0) rayPayloadInEXT RayPayload payload;

void main() {
    vec3 position = get_vertex_position(gl_InstanceID, barycentrics);
    vec3 normal = get_vertex_normal(gl_InstanceID, barycentrics);
    vec2 uv = get_vertex_uv(gl_InstanceID, barycentrics);

    payload.hit = true;
    payload.instance = gl_InstanceID;
    payload.position = gl_ObjectToWorldEXT * vec4(position, 1.0);
    payload.normal = normal;
    payload.uv = uv;
}