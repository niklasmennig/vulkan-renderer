float balance_heuristic(float n_f, float p_f, float n_g, float p_g) {
    return (n_f * p_f) / (n_f * p_f + n_g * p_g);
}