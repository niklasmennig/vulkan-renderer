// https://research.nvidia.com/publication/2020-07_spatiotemporal-reservoir-resampling-real-time-ray-tracing-dynamic-direct
// https://research.nvidia.com/sites/default/files/pubs/2020-07_Spatiotemporal-reservoir-resampling/ReSTIR.pdf
struct Reservoir {
    vec3 contribution;
    float sum_weights;
    uint num_samples;
};

#ifndef SHADER_STRUCTS_ONLY

void update_reservoir(Reservoir r, vec3 contribution, float weight) {
    r.sum_weights = r.sum_weights + weight;
    r.num_samples = r.num_samples + 1;
    if (random_float(0) < weight / r.sum_weights) {
        r.contribution = contribution;
    }
}

void combine_reservoirs(Reservoir r_0, Reservoir r_1) {
    uint old_samples = r_0.num_samples;
    update_reservoir(r_0, r_1.contribution, r_1.sum_weights * r_1.num_samples);
    r_0.num_samples = old_samples + r_1.num_samples;
    r_0.sum_weights = r_0.sum_weights + r_1.sum_weights;
}

#endif