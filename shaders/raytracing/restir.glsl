#ifndef RESTIR_GLSL
#define RESTIR_GLSL

// https://research.nvidia.com/publication/2020-07_spatiotemporal-reservoir-resampling-real-time-ray-tracing-dynamic-direct
// https://research.nvidia.com/sites/default/files/pubs/2020-07_Spatiotemporal-reservoir-resampling/ReSTIR.pdf


// reservoirs are continuously resampled and updated to find good NEE paths to reproduce
struct Reservoir {
    uint num_samples;
    float sum_weights;
    uint sample_seed;
    float padding;
};

#ifndef SHADER_STRUCTS_ONLY

void update_reservoir(inout Reservoir r, uint sample_seed, float weight, inout uint seed) {
    r.sum_weights = r.sum_weights + weight;
    r.num_samples = r.num_samples + 1;
    if (random_float(seed) < weight / r.sum_weights) {
        r.sample_seed = sample_seed;
    }
}

void combine_reservoirs(inout Reservoir r, Reservoir r1, inout uint seed) {
    uint old_samples = r.num_samples;
    update_reservoir(r, r1.sample_seed, r1.sum_weights, seed);
    r.num_samples = old_samples + r1.num_samples;
    r.sum_weights = r.sum_weights + r1.sum_weights;
}

void clear_reservoir(inout Reservoir r) {
    r.sum_weights = 0;
    r.num_samples = 0;
}

#endif
#endif