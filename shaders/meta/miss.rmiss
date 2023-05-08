#version 460 core
#extension GL_EXT_ray_tracing : enable

struct RayPayload
{
    float hit_t;
    vec3 hit_position;
    vec3 hit_normal;
    vec2 hit_uv;
    uint hit_instance;
    uint hit_primitive;
};
layout(location = 0) rayPayloadInEXT RayPayload payload;

void main() {
    payload.hit_t = -1;
}