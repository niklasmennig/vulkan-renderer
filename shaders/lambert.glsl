#include "common.glsl"
#include "sampling.glsl"

BSDFSample sample_lambert(in vec3 V, in mat3 tbn, in vec3 baseColor, in vec4 random) {
        DirectionSample dir_sample = sample_cosine_hemisphere(random.x, random.y);

        BSDFSample result;
        result.contribution = baseColor;
        result.direction = tbn * dir_sample.direction;
        result.pdf = dir_sample.pdf;

        return result;
}

vec3 eval_lambert(in vec3 baseColor) {
        return baseColor;
}