#include "common.glsl"

vec3 sample_lambert(in vec3 V, in mat3 tbn, in vec3 baseColor, in vec4 random, out vec3 nextFactor) {
        nextFactor = baseColor;

        float theta = acos(random.x);
        float phi = 2.0 * PI * random.y;

        vec3 dir = vec3(cos(phi) * sin(theta), sin(phi) * sin(theta), cos(theta));

        return tbn * dir;
}