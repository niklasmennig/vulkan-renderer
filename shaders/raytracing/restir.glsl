#ifndef RESTIR_GLSL
#define RESTIR_GLSL

// https://research.nvidia.com/publication/2020-07_spatiotemporal-reservoir-resampling-real-time-ray-tracing-dynamic-direct
// https://research.nvidia.com/sites/default/files/pubs/2020-07_Spatiotemporal-reservoir-resampling/ReSTIR.pdf

// sample contains good NEE seed
struct RestirSample {
    vec4 light_direction;
    vec4 light_intensity;
};

// reservoirs are continuously resampled and updated to find good NEE paths to reproduce
struct Reservoir {
    RestirSample y;
    float sum_weights;
    uint num_samples;
    vec2 padding;
};

#ifndef SHADER_STRUCTS_ONLY

void update_reservoir(inout Reservoir r, RestirSample nee_sample, float weight, inout uint seed) {
    r.sum_weights = r.sum_weights + weight;
    r.num_samples = r.num_samples + 1;
    if (random_float(seed) < weight / r.sum_weights) {
        r.y = nee_sample;
    }
}

void combine_reservoirs(inout Reservoir r_0, Reservoir r_1, inout uint seed) {
    uint old_samples = r_0.num_samples;
    update_reservoir(r_0, r_1.y, r_1.sum_weights * r_1.num_samples, seed);
    r_0.num_samples = old_samples + r_1.num_samples;
    r_0.sum_weights = r_0.sum_weights + r_1.sum_weights;
}

void clear_reservoir(inout Reservoir r) {
    r.sum_weights = 0;
    r.num_samples = 0;
}

#endif
#endif