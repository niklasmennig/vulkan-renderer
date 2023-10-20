#version 460
#extension GL_EXT_ray_tracing : enable

#include "payload.glsl"

hitAttributeEXT vec2 barycentrics;

layout(location = 0) rayPayloadInEXT RayPayload payload;


void main() {
    payload.hit = true;
    payload.hit_instance = gl_InstanceID;
    payload.hit_primitive = gl_PrimitiveID;
    payload.hit_transform_world = gl_ObjectToWorldEXT;
    payload.hit_transform_object = gl_WorldToObjectEXT;
    payload.hit_barycentrics = barycentrics;
}