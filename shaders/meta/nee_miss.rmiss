#version 460 core
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : require



layout(location = 1) rayPayloadInEXT bool nee_miss;

void main() {
    nee_miss = true;
}