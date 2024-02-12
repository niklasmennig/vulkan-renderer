#version 460
#extension GL_EXT_ray_tracing : enable

layout(location = 1) rayPayloadInEXT bool shadowray_occluded;

void main() {
    shadowray_occluded = true;
}