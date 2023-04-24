#version 460 core
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_buffer_reference2 : require

~include "shaders/payload.glsl"

layout(location = 0) rayPayloadInEXT RayPayload payload;
layout(set = 0, binding = 3) readonly buffer IndexData {uint vertex_indices[];} index_data;
layout(set = 0, binding = 4) readonly buffer MeshData {vec4 vertices[];} mesh_data;

void main() {
    payload.distance = gl_HitTEXT;

    uint normal_index = index_data.vertex_indices[gl_PrimitiveID * 3];
    vec3 normal = mesh_data.vertices[normal_index].xyz;
    
    vec3 ray_dir = gl_ObjectRayDirectionEXT;
    float cos_term = abs(dot(normalize(normal), normalize(ray_dir)));

    vec3 color = vec3(1,0,0);
    color *= cos_term;
    payload.contribution = vec4(color,1.0);
}