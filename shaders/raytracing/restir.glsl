// https://research.nvidia.com/publication/2020-07_spatiotemporal-reservoir-resampling-real-time-ray-tracing-dynamic-direct
// https://research.nvidia.com/sites/default/files/pubs/2020-07_Spatiotemporal-reservoir-resampling/ReSTIR.pdf

// sample contains good NEE seed
struct Sample {
    uint nee_seed;
};

// reservoirs are continuously resampled and updated to find good NEE seeds to reproduce
struct Reservoir {
    Sample y;
    float sum_weights;
    uint num_samples;
    float padding;
};

#ifndef SHADER_STRUCTS_ONLY

void update_reservoir(Reservoir r, uint nee_seed, float weight, uint seed) {
    r.sum_weights = r.sum_weights + weight;
    r.num_samples = r.num_samples + 1;
    if (random_float(seed) < weight / r.sum_weights) {
        r.y.nee_seed = nee_seed;
    }
}

void combine_reservoirs(Reservoir r_0, Reservoir r_1, uint seed) {
    uint old_samples = r_0.num_samples;
    update_reservoir(r_0, r_1.y.nee_seed, r_1.sum_weights * r_1.num_samples, seed);
    r_0.num_samples = old_samples + r_1.num_samples;
    r_0.sum_weights = r_0.sum_weights + r_1.sum_weights;
}

void clear_reservoir(Reservoir r) {
    r.y.nee_seed = 0;
    r.sum_weights = 0;
    r.num_samples = 0;
}

#endif