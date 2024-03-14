#ifndef RESTIR_GLSL
#define RESTIR_GLSL


// https://research.nvidia.com/publication/2020-07_spatiotemporal-reservoir-resampling-real-time-ray-tracing-dynamic-direct
// https://research.nvidia.com/sites/default/files/pubs/2020-07_Spatiotemporal-reservoir-resampling/ReSTIR.pdf


// reservoirs are continuously resampled and updated to find good NEE paths to reproduce
struct Reservoir {
    uint num_samples;
    float sum_weights;
    uint sample_seed;
    float weight;
};

#ifndef SHADER_STRUCTS_ONLY
#include "color.glsl"
#include "structs.glsl"

void update_reservoir(inout Reservoir r, uint sample_seed, float weight, inout uint seed) {
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

float restir_phat(LightSample light_sample, vec3 normal) {
    return luminance(light_sample.weight) * abs(dot(light_sample.direction, normal));
}

#endif
#endif