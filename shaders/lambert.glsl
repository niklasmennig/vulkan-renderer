#include "common.glsl"
#include "sampling.glsl"

BSDFSample sample_lambert(in vec3 V, in mat3 tbn, in vec3 baseColor, in vec4 random) {
        vec3 sample_direction = sample_cosine_hemisphere(random.x, random.y);

        BSDFSample result;
        result.contribution = baseColor;
        result.direction = tbn * sample_direction;
        result.pdf = 1.0f / PI;

        return result;
}

vec3 eval_lambert(in vec3 baseColor) {
        return baseColor;
}