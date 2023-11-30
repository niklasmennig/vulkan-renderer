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

uint sample_tea(inout uint v0, inout uint v1) {
    uint sum = 0;
    sum += 0x9e3779b9;
    v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + sum) ^ ((v1 >> 5) + 0xc8013ea4);
    v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + sum) ^ ((v0 >> 5) + 0x7e95761e);
    sum += 0x9e3779b9;
    v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + sum) ^ ((v1 >> 5) + 0xc8013ea4);
    v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + sum) ^ ((v0 >> 5) + 0x7e95761e);
    sum += 0x9e3779b9;
    v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + sum) ^ ((v1 >> 5) + 0xc8013ea4);
    v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + sum) ^ ((v0 >> 5) + 0x7e95761e);
    sum += 0x9e3779b9;
    v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + sum) ^ ((v1 >> 5) + 0xc8013ea4);
    v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + sum) ^ ((v0 >> 5) + 0x7e95761e);

    return v1;
}

uint prng_uint(uint seed, uint offset) {
    return hash_combine(hash_combine(hash_init, seed), offset);
}

float prng_float(uint seed, uint offset) {
    uint x = prng_uint(seed, offset);
    return uintBitsToFloat((x & 0x7FFFFF) | 0x3F800000) - 1;
}

float random_float(inout uint seed) {
    seed = prng_uint(seed, 0);
    return prng_float(seed, 0);
}

#endif