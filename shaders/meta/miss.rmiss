#version 460 core
#extension GL_EXT_ray_tracing : enable

struct RayPayload
{
    vec4 contribution;
    float distance;
};
layout(location = 0) rayPayloadInEXT RayPayload payload;

void main() {
    payload.contribution = vec4(0,0,0,1);
    payload.distance = 99999.0;
}