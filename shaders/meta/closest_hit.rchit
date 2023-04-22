#version 460 core
#extension GL_EXT_ray_tracing : enable
layout(location = 0) rayPayloadInEXT vec4 payload;
layout(set = 0, binding = 1) uniform accelerationStructureEXT as;

void main() {
  
    payload = vec4(1,0,0,1);
}