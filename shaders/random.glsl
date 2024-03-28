#ifndef RANDOM_GLSL
#define RANDOM_GLSL

const uint hash_init = 0x811C9DC5;

uint hash_combine(uint h, uint d) {
    h = (h * 16777619) ^ ( d        & 0xFF);
    h = (h * 16777619) ^ ((d >>  8) & 0xFF);
    h = (h * 16777619) ^ ((d >> 16) & 0xFF);
    h = (h * 16777619) ^ ((d >> 24) & 0xFF);

    return h;
}

uint random_uint(inout uint seed) {
    uint m = 1664525;
    uint n = 1013904223;
    seed = seed * m + n;
    return seed;
}

float random_float(inout uint seed) {
    return uintBitsToFloat((random_uint(seed) & 0x7FFFFF) | 0x3F800000) - 1;
}

vec3 random_vec3(inout uint seed) {
    return vec3(random_float(seed), random_float(seed), random_float(seed));
}

#endif