#ifndef RESTIR_GLSL
#define RESTIR_GLSL

// https://research.nvidia.com/publication/2020-07_spatiotemporal-reservoir-resampling-real-time-ray-tracing-dynamic-direct
// https://research.nvidia.com/sites/default/files/pubs/2020-07_Spatiotemporal-reservoir-resampling/ReSTIR.pdf
#include "structs.glsl"
#include "lights.glsl"
#include "output.glsl"


layout(std430, set = DESCRIPTOR_SET_CUSTOM, binding = DESCRIPTOR_BINDING_RESTIR_RESERVOIRS) buffer ReSTIRReservoirBuffers {Reservoir reservoirs[];} restir_reservoirs[];

// reservoirs are continuously resampled and updated to find good NEE paths to reproduce
float restir_p_hat(uint pixel_index, uint seed) {
    vec3 position = read_output(OUTPUT_BUFFER_POSITION, pixel_index).xyz;
    vec3 normal = read_output(OUTPUT_BUFFER_NORMAL, pixel_index).xyz;
    LightSample light_sample = sample_direct_light(seed, position);
    return luminance(light_sample.weight) * abs(dot(light_sample.direction, normal));
}

void update_reservoir(inout Reservoir r, uint sample_seed, float weight, uint seed) {
    r.sum_weights = r.sum_weights + weight;
    r.num_samples = r.num_samples + 1;
    if (random_float(seed) < weight / r.sum_weights) {
        r.sample_seed = sample_seed;
    }
}

void clear_reservoir(inout Reservoir r) {
    r.sum_weights = 0.0;
    r.num_samples = 0;
    r.sample_seed = 0;
    r.weight = 0.0;
}

Reservoir combine_reservoirs(uint pixel_index, Reservoir r0, Reservoir r1, uint seed) {
    Reservoir s;
    clear_reservoir(s);

    update_reservoir(s, r0.sample_seed, restir_p_hat(pixel_index, r0.sample_seed) * r0.weight * r0.num_samples, random_uint(seed));
    update_reservoir(s, r1.sample_seed, restir_p_hat(pixel_index, r1.sample_seed) * r1.weight * r1.num_samples, random_uint(seed));

    s.num_samples = r0.num_samples + r1.num_samples;
    s.weight = (1.0 / restir_p_hat(pixel_index, s.sample_seed)) * (s.sum_weights / s.num_samples);

    return s;
}



#endif